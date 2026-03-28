#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <elf.h>
#include "elf_fusion_symbols.h"
#include "elf_fusion_internal.h"
#include "elf_lecture.h"
#include "elf_affichage_symboles.h"

// ajoute un nom dans la strtab et retourne son offset
static Elf32_Word ajouter_nom_strtab(char **strtab, size_t *strtab_size, const char *nom){
    size_t nom_len = strlen(nom) + 1;
    size_t old_size = *strtab_size;
    
    *strtab = realloc(*strtab, old_size + nom_len);
    if (!*strtab)
        return 0;
    
    memcpy(*strtab + old_size, nom, nom_len);
    *strtab_size = old_size + nom_len;
    
    return old_size;
}


static int symbole_est_defini(const Elf32_Sym *sym){
    return sym->st_shndx != SHN_UNDEF;
}


static int symbole_est_global(const Elf32_Sym *sym){
    unsigned char bind = ELF32_ST_BIND(sym->st_info);
    return (bind == STB_GLOBAL || bind == STB_WEAK);
}

// pour qsort: trie les symboles globaux par valeur
static int comparer_symboles_globaux(const void *a, const void *b){
    const Elf32_Sym *sym_a = (const Elf32_Sym *)a;
    const Elf32_Sym *sym_b = (const Elf32_Sym *)b;
    
    int def_a = (sym_a->st_shndx != SHN_UNDEF) ? 1 : 0;
    int def_b = (sym_b->st_shndx != SHN_UNDEF) ? 1 : 0;
    
    // definis d'abord, puis non definis
    if (def_a != def_b)
        return def_b - def_a;
    
    if (sym_a->st_value < sym_b->st_value)
        return -1;
    if (sym_a->st_value > sym_b->st_value)
        return 1;
    
    return 0;
}

// copie un symbole en corrigeant section et valeur
static void copier_symbole_corrige(Elf32_Sym *dest, const Elf32_Sym *src,
                                   const char *nom_src, char **strtab, 
                                   size_t *strtab_size,
                                   int from_file2, resultat_fusion_t *res_sections){
    *dest = *src;
    
    dest->st_name = ajouter_nom_strtab(strtab, strtab_size, nom_src);
    
    // corriger st_shndx et st_value
    if (symbole_est_defini(src) && src->st_shndx < SHN_LORESERVE) {
        int old_shndx = src->st_shndx;
        
        if (from_file2) {
            // fichier2: renumeroter avec map2 et ajouter delta
            if (old_shndx < res_sections->map2_size && 
                res_sections->map2[old_shndx] >= 0) {
                
                dest->st_shndx = res_sections->map2[old_shndx];
                dest->st_value = src->st_value + res_sections->delta2[old_shndx];
            }
        } else {
            // fichier1: renumeroter avec map1, pas de delta
            if (old_shndx < res_sections->map1_size && 
                res_sections->map1[old_shndx] >= 0) {
                
                dest->st_shndx = res_sections->map1[old_shndx];
            }
        }
    }
}

// fusionne les tables de symboles de 2 fichiers
int fusion_etape7(char *fichier1, char *fichier2,
                  resultat_fusion_t *resultat_sections,
                  resultat_symboles_t *resultat_symboles){
    elf_contexte_t ctx1, ctx2;
    SymbolTable symtab1, symtab2;
    int ret = -1;
    
    memset(&symtab1, 0, sizeof(SymbolTable));
    memset(&symtab2, 0, sizeof(SymbolTable));
    memset(resultat_symboles, 0, sizeof(resultat_symboles_t));
    
    
    if (elf_ouvrir(fichier1, &ctx1) != 0) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", fichier1);
        return -1;
    }
    
    if (elf_ouvrir(fichier2, &ctx2) != 0) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", fichier2);
        elf_fermer(&ctx1);
        return -1;
    }
    
    
    if (elf_lire_symboles(&ctx1, &symtab1) != 0) {
        fprintf(stderr, "Erreur: impossible de lire les symboles de %s\n", fichier1);
        goto cleanup;
    }
    
    if (elf_lire_symboles(&ctx2, &symtab2) != 0) {
        fprintf(stderr, "Erreur: impossible de lire les symboles de %s\n", fichier2);
        goto cleanup;
    }
    
    
    resultat_symboles->sym_map1_size = symtab1.size;
    resultat_symboles->sym_map2_size = symtab2.size;
    resultat_symboles->sym_map1 = malloc(symtab1.size * sizeof(int));
    resultat_symboles->sym_map2 = malloc(symtab2.size * sizeof(int));
    
    if (!resultat_symboles->sym_map1 || !resultat_symboles->sym_map2) {
        fprintf(stderr, "Erreur: allocation mémoire\n");
        goto cleanup;
    }
    
    
    for (size_t i = 0; i < symtab1.size; i++)
        resultat_symboles->sym_map1[i] = -1;
    for (size_t i = 0; i < symtab2.size; i++)
        resultat_symboles->sym_map2[i] = -1;
    
    
    int max_symbols = symtab1.size + symtab2.size;
    resultat_symboles->symbols = calloc(max_symbols, sizeof(Elf32_Sym));
    if (!resultat_symboles->symbols) {
        fprintf(stderr, "Erreur: allocation mémoire\n");
        goto cleanup;
    }
    
    
    resultat_symboles->strtab = malloc(1);
    if (!resultat_symboles->strtab) {
        fprintf(stderr, "Erreur: allocation mémoire\n");
        goto cleanup;
    }
    resultat_symboles->strtab[0] = '\0';
    resultat_symboles->strtab_size = 1;
    
    int nb_symbols = 0;
    
    // 1. symbole NULL obligatoire en premier
    memset(&resultat_symboles->symbols[nb_symbols], 0, sizeof(Elf32_Sym));
    resultat_symboles->symbols[nb_symbols].st_name = ajouter_nom_strtab(
        &resultat_symboles->strtab, &resultat_symboles->strtab_size, "");
    nb_symbols++;
    
    // 2. symboles SECTION pour chaque section fusionnee
    for (int i = 0; i < resultat_sections->nb_sections; i++) {
        Elf32_Sym *sym = &resultat_symboles->symbols[nb_symbols];
        memset(sym, 0, sizeof(Elf32_Sym));
        
        sym->st_name = ajouter_nom_strtab(&resultat_symboles->strtab,
                                         &resultat_symboles->strtab_size,
                                         resultat_sections->sections[i].nom);
        sym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
        sym->st_other = 0;
        sym->st_shndx = i + 1; 
        sym->st_value = 0;
        sym->st_size = 0;
        
        
        section_fusionnee_t *sec = &resultat_sections->sections[i];
        
        
        if (sec->shndx_file1 >= 0) {
            for (size_t j = 0; j < symtab1.size; j++) {
                if (ELF32_ST_TYPE(symtab1.symbols[j].st_info) == STT_SECTION &&
                    symtab1.symbols[j].st_shndx == sec->shndx_file1) {
                    resultat_symboles->sym_map1[j] = nb_symbols;
                    break;
                }
            }
        }
        
        
        if (sec->shndx_file2 >= 0) {
            for (size_t j = 0; j < symtab2.size; j++) {
                if (ELF32_ST_TYPE(symtab2.symbols[j].st_info) == STT_SECTION &&
                    symtab2.symbols[j].st_shndx == sec->shndx_file2) {
                    resultat_symboles->sym_map2[j] = nb_symbols;
                    break;
                }
            }
        }
        
        nb_symbols++;
    }
    
    
    
    for (size_t i = 0; i < symtab1.size; i++) {
        const Elf32_Sym *sym = &symtab1.symbols[i];
        if (ELF32_ST_TYPE(sym->st_info) == STT_SECTION && sym->st_shndx < ctx1.entete.e_shnum) {
            const char *nom_section = ctx1.noms_sections + ctx1.sections[sym->st_shndx].sh_name;
            Elf32_Word type = ctx1.sections[sym->st_shndx].sh_type;
            
            
            if (type == SHT_PROGBITS || strstr(nom_section, ".debug") != NULL)
                continue;
            
            
            int deja_presente = 0;
            for (int j = 1; j < nb_symbols; j++) {
                if (ELF32_ST_TYPE(resultat_symboles->symbols[j].st_info) == STT_SECTION) {
                    const char *nom_existant = resultat_symboles->strtab + resultat_symboles->symbols[j].st_name;
                    if (strcmp(nom_existant, nom_section) == 0) {
                        deja_presente = 1;
                        break;
                    }
                }
            }
            
            if (!deja_presente) {
                Elf32_Sym *new_sym = &resultat_symboles->symbols[nb_symbols];
                memset(new_sym, 0, sizeof(Elf32_Sym));
                
                new_sym->st_name = ajouter_nom_strtab(&resultat_symboles->strtab,
                                                     &resultat_symboles->strtab_size,
                                                     nom_section);
                new_sym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
                new_sym->st_other = 0;
                new_sym->st_shndx = nb_symbols; 
                new_sym->st_value = 0;
                new_sym->st_size = 0;
                
                
                resultat_symboles->sym_map1[i] = nb_symbols;
                
                nb_symbols++;
            } else {
                
                for (int j = 1; j < nb_symbols; j++) {
                    if (ELF32_ST_TYPE(resultat_symboles->symbols[j].st_info) == STT_SECTION) {
                        const char *nom_existant = resultat_symboles->strtab + resultat_symboles->symbols[j].st_name;
                        if (strcmp(nom_existant, nom_section) == 0) {
                            resultat_symboles->sym_map1[i] = j;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    
    for (size_t i = 0; i < symtab2.size; i++) {
        const Elf32_Sym *sym = &symtab2.symbols[i];
        if (ELF32_ST_TYPE(sym->st_info) == STT_SECTION && sym->st_shndx < ctx2.entete.e_shnum) {
            const char *nom_section = ctx2.noms_sections + ctx2.sections[sym->st_shndx].sh_name;
            Elf32_Word type = ctx2.sections[sym->st_shndx].sh_type;
            
            if (type == SHT_PROGBITS || strstr(nom_section, ".debug") != NULL)
                continue;
            
            int deja_presente = 0;
            for (int j = 1; j < nb_symbols; j++) {
                if (ELF32_ST_TYPE(resultat_symboles->symbols[j].st_info) == STT_SECTION) {
                    const char *nom_existant = resultat_symboles->strtab + resultat_symboles->symbols[j].st_name;
                    if (strcmp(nom_existant, nom_section) == 0) {
                        deja_presente = 1;
                        break;
                    }
                }
            }
            
            if (!deja_presente) {
                Elf32_Sym *new_sym = &resultat_symboles->symbols[nb_symbols];
                memset(new_sym, 0, sizeof(Elf32_Sym));
                
                new_sym->st_name = ajouter_nom_strtab(&resultat_symboles->strtab,
                                                     &resultat_symboles->strtab_size,
                                                     nom_section);
                new_sym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
                new_sym->st_other = 0;
                new_sym->st_shndx = nb_symbols;
                new_sym->st_value = 0;
                new_sym->st_size = 0;
                
                
                resultat_symboles->sym_map2[i] = nb_symbols;
                
                nb_symbols++;
            } else {
                
                for (int j = 1; j < nb_symbols; j++) {
                    if (ELF32_ST_TYPE(resultat_symboles->symbols[j].st_info) == STT_SECTION) {
                        const char *nom_existant = resultat_symboles->strtab + resultat_symboles->symbols[j].st_name;
                        if (strcmp(nom_existant, nom_section) == 0) {
                            resultat_symboles->sym_map2[i] = j;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    
    int has_file1 = 0;
    for (size_t i = 0; i < symtab1.size; i++) {
        if (ELF32_ST_TYPE(symtab1.symbols[i].st_info) == STT_FILE) {
            has_file1 = 1;
            break;
        }
    }
    
    if (!has_file1) {
        Elf32_Sym *sym = &resultat_symboles->symbols[nb_symbols];
        memset(sym, 0, sizeof(Elf32_Sym));
        
        const char *nom_fichier1 = strrchr(fichier1, '/');
        nom_fichier1 = nom_fichier1 ? nom_fichier1 + 1 : fichier1;
        
        sym->st_name = ajouter_nom_strtab(&resultat_symboles->strtab,
                                         &resultat_symboles->strtab_size,
                                         nom_fichier1);
        sym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_FILE);
        sym->st_other = 0;
        sym->st_shndx = SHN_ABS;
        sym->st_value = 0;
        sym->st_size = 0;
        
        nb_symbols++;
    }
    
    
    for (size_t i = 0; i < symtab1.size; i++) {
        const Elf32_Sym *sym = &symtab1.symbols[i];
        unsigned type = ELF32_ST_TYPE(sym->st_info);
        
        
        if (i == 0 || type == STT_SECTION)
            continue;
        
        if (!symbole_est_global(sym)) {
            const char *nom = symtab1.strtab + sym->st_name;
            
            copier_symbole_corrige(&resultat_symboles->symbols[nb_symbols],
                                  sym, nom,
                                  &resultat_symboles->strtab,
                                  &resultat_symboles->strtab_size,
                                  0, resultat_sections);
            
            resultat_symboles->sym_map1[i] = nb_symbols;
            nb_symbols++;
        }
    }
    
    
    int has_file2 = 0;
    for (size_t i = 0; i < symtab2.size; i++) {
        if (ELF32_ST_TYPE(symtab2.symbols[i].st_info) == STT_FILE) {
            has_file2 = 1;
            break;
        }
    }
    
    if (!has_file2) {
        Elf32_Sym *sym = &resultat_symboles->symbols[nb_symbols];
        memset(sym, 0, sizeof(Elf32_Sym));
        
        const char *nom_fichier2 = strrchr(fichier2, '/');
        nom_fichier2 = nom_fichier2 ? nom_fichier2 + 1 : fichier2;
        
        sym->st_name = ajouter_nom_strtab(&resultat_symboles->strtab,
                                         &resultat_symboles->strtab_size,
                                         nom_fichier2);
        sym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_FILE);
        sym->st_other = 0;
        sym->st_shndx = SHN_ABS;
        sym->st_value = 0;
        sym->st_size = 0;
        
        nb_symbols++;
    }
    
    
    for (size_t i = 0; i < symtab2.size; i++) {
        const Elf32_Sym *sym = &symtab2.symbols[i];
        unsigned type = ELF32_ST_TYPE(sym->st_info);
        
        
        if (i == 0 || type == STT_SECTION)
            continue;
        
        if (!symbole_est_global(sym)) {
            const char *nom = symtab2.strtab + sym->st_name;
            
            copier_symbole_corrige(&resultat_symboles->symbols[nb_symbols],
                                  sym, nom,
                                  &resultat_symboles->strtab,
                                  &resultat_symboles->strtab_size,
                                  1, resultat_sections);
            
            resultat_symboles->sym_map2[i] = nb_symbols;
            nb_symbols++;
        }
    }
    
    
    for (int i = 0; i < resultat_sections->nb_sections; i++) {
        if (strcmp(resultat_sections->sections[i].nom, ".data") == 0 && 
            resultat_sections->sections[i].size > 0) {
            Elf32_Sym *sym = &resultat_symboles->symbols[nb_symbols];
            memset(sym, 0, sizeof(Elf32_Sym));
            
            sym->st_name = ajouter_nom_strtab(&resultat_symboles->strtab,
                                             &resultat_symboles->strtab_size,
                                             "$d");
            sym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE);
            sym->st_other = 0;
            sym->st_shndx = i + 1; 
            sym->st_value = 0;
            sym->st_size = 0;
            
            nb_symbols++;
            break;
        }
    }
    
    
    for (size_t i = 0; i < symtab1.size; i++) {
        const Elf32_Sym *sym = &symtab1.symbols[i];
        
        if (symbole_est_global(sym)) {
            const char *nom = symtab1.strtab + sym->st_name;
            int defini1 = symbole_est_defini(sym);
            
            
            int idx2 = -1;
            int defini2 = 0;
            
            for (size_t j = 0; j < symtab2.size; j++) {
                const Elf32_Sym *sym2 = &symtab2.symbols[j];
                if (!symbole_est_global(sym2))
                    continue;
                
                const char *nom2 = symtab2.strtab + sym2->st_name;
                if (strcmp(nom, nom2) == 0) {
                    idx2 = j;
                    defini2 = symbole_est_defini(sym2);
                    break;
                }
            }
            
            
            if (defini1 && defini2) {
                fprintf(stderr, "Erreur: symbole '%s' défini dans les deux fichiers\n", nom);
                goto cleanup;
            }
            
            
            if (defini1) {
                copier_symbole_corrige(&resultat_symboles->symbols[nb_symbols],
                                      sym, nom,
                                      &resultat_symboles->strtab,
                                      &resultat_symboles->strtab_size,
                                      0, resultat_sections);
                
                resultat_symboles->sym_map1[i] = nb_symbols;
                if (idx2 >= 0)
                    resultat_symboles->sym_map2[idx2] = nb_symbols;
                
                nb_symbols++;
            }
            
            else if (idx2 >= 0 && defini2) {
                const Elf32_Sym *sym2 = &symtab2.symbols[idx2];
                
                copier_symbole_corrige(&resultat_symboles->symbols[nb_symbols],
                                      sym2, nom,
                                      &resultat_symboles->strtab,
                                      &resultat_symboles->strtab_size,
                                      1, resultat_sections);
                
                resultat_symboles->sym_map1[i] = nb_symbols;
                resultat_symboles->sym_map2[idx2] = nb_symbols;
                nb_symbols++;
            }
            
            else {
                copier_symbole_corrige(&resultat_symboles->symbols[nb_symbols],
                                      sym, nom,
                                      &resultat_symboles->strtab,
                                      &resultat_symboles->strtab_size,
                                      0, resultat_sections);
                
                resultat_symboles->sym_map1[i] = nb_symbols;
                if (idx2 >= 0)
                    resultat_symboles->sym_map2[idx2] = nb_symbols;
                
                nb_symbols++;
            }
        }
    }
    
    
    for (size_t i = 0; i < symtab2.size; i++) {
        const Elf32_Sym *sym = &symtab2.symbols[i];
        
        if (symbole_est_global(sym) && resultat_symboles->sym_map2[i] == -1) {
            const char *nom = symtab2.strtab + sym->st_name;
            
            copier_symbole_corrige(&resultat_symboles->symbols[nb_symbols],
                                  sym, nom,
                                  &resultat_symboles->strtab,
                                  &resultat_symboles->strtab_size,
                                  1, resultat_sections);
            
            resultat_symboles->sym_map2[i] = nb_symbols;
            nb_symbols++;
        }
    }
    
    resultat_symboles->nb_symbols = nb_symbols;
    
    
    int premier_global = -1;
    int nb_globaux = 0;
    
    
    for (int i = 0; i < nb_symbols; i++) {
        if (symbole_est_global(&resultat_symboles->symbols[i])) {
            if (premier_global == -1)
                premier_global = i;
            nb_globaux++;
        }
    }
    
    if (premier_global >= 0 && nb_globaux > 1) {
        Elf32_Sym *symboles_avant_tri = malloc(nb_globaux * sizeof(Elf32_Sym));
        if (!symboles_avant_tri) {
            fprintf(stderr, "Erreur: allocation mémoire pour tri\n");
            goto cleanup;
        }
        memcpy(symboles_avant_tri, &resultat_symboles->symbols[premier_global],
               nb_globaux * sizeof(Elf32_Sym));
        
        qsort(&resultat_symboles->symbols[premier_global], 
              nb_globaux, 
              sizeof(Elf32_Sym), 
              comparer_symboles_globaux);
        
        int *permutation = malloc(nb_globaux * sizeof(int));
        if (!permutation) {
            free(symboles_avant_tri);
            fprintf(stderr, "Erreur: allocation mémoire pour permutation\n");
            goto cleanup;
        }
        
        for (int i = 0; i < nb_globaux; i++) {
            Elf32_Sym *sym_avant = &symboles_avant_tri[i];
            
            for (int j = 0; j < nb_globaux; j++) {
                Elf32_Sym *sym_apres = &resultat_symboles->symbols[premier_global + j];
                
                if (sym_avant->st_value == sym_apres->st_value &&
                    sym_avant->st_name == sym_apres->st_name &&
                    sym_avant->st_shndx == sym_apres->st_shndx) {
                    permutation[i] = j;
                    break;
                }
            }
        }
        
        
        for (size_t i = 0; i < symtab1.size; i++) {
            if (resultat_symboles->sym_map1[i] >= premier_global &&
                resultat_symboles->sym_map1[i] < premier_global + nb_globaux) {
                int idx_relatif = resultat_symboles->sym_map1[i] - premier_global;
                resultat_symboles->sym_map1[i] = premier_global + permutation[idx_relatif];
            }
        }
        
        for (size_t i = 0; i < symtab2.size; i++) {
            if (resultat_symboles->sym_map2[i] >= premier_global &&
                resultat_symboles->sym_map2[i] < premier_global + nb_globaux) {
                int idx_relatif = resultat_symboles->sym_map2[i] - premier_global;
                resultat_symboles->sym_map2[i] = premier_global + permutation[idx_relatif];
            }
        }
        
        free(permutation);
        free(symboles_avant_tri);
    }
    
    ret = 0;

cleanup:
    elf_liberer_symboles(&symtab1);
    elf_liberer_symboles(&symtab2);
    elf_fermer(&ctx1);
    elf_fermer(&ctx2);
    
    if (ret != 0) {
        liberer_resultat_symboles(resultat_symboles);
    }
    
    return ret;
}


void liberer_resultat_symboles(resultat_symboles_t *resultat){
    if (resultat->symbols)
        free(resultat->symbols);
    
    if (resultat->strtab)
        free(resultat->strtab);
    
    if (resultat->sym_map1)
        free(resultat->sym_map1);
    
    if (resultat->sym_map2)
        free(resultat->sym_map2);
    
    memset(resultat, 0, sizeof(resultat_symboles_t));
}



void afficher_resultat_symboles(resultat_symboles_t *resultat){
    
    
    SymbolTable symtab_temp;
    symtab_temp.symbols = resultat->symbols;
    symtab_temp.size = resultat->nb_symbols;
    symtab_temp.strtab = resultat->strtab;
    
    
    elf_afficher_symboles(NULL, &symtab_temp);
    
    printf("\nRenumérotation symboles fichier1 :\n");
    for (int i = 0; i < resultat->sym_map1_size; i++) {
        if (resultat->sym_map1[i] >= 0) {
            printf("  Symbole [%d] -> [%d]\n", i, resultat->sym_map1[i]);
        }
    }
    
    
    printf("\nTaille strtab : %zu octets\n", resultat->strtab_size);
    
    //printf("========================================\n");
}
