#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <elf.h>
#include "elf_fusion_relocations.h"
#include "elf_fusion_internal.h"
#include "elf_lecture.h"
#include "elf_affichage_symboles.h"
#include "elf_affichage_relocations.h"
#include "util.h"


// libere la memoire des relocations
void liberer_resultat_relocations(resultat_relocations_t *resultat){
    if (resultat->sections) {
        for (int i = 0; i < resultat->nb_sections; i++) {
            if (resultat->sections[i].name)
                free(resultat->sections[i].name);
            if (resultat->sections[i].entries)
                free(resultat->sections[i].entries);
        }
        free(resultat->sections);
    }
    
    memset(resultat, 0, sizeof(resultat_relocations_t));
}
// retourne le nom du type de relocation ARM
static const char *arm_reloc_type_name(unsigned int type){
    switch (type) {
        case 0:  return "R_ARM_NONE";
        case 1:  return "R_ARM_PC24";
        case 2:  return "R_ARM_ABS32";
        case 3:  return "R_ARM_REL32";
        case 28: return "R_ARM_CALL";
        case 29: return "R_ARM_JUMP24";
        case 40: return "R_ARM_V4BX";
        default: return "UNKNOWN";
    }
}
// affiche les relocations fusionnees pour debug
void afficher_resultat_relocations(resultat_relocations_t *resultat,
                                   resultat_symboles_t *resultat_symboles){
    printf("\n========================================\n");
    printf("  ETAPE 8 : FUSION DES RELOCATIONS\n");
    printf("========================================\n\n");
    
    printf("Nombre de sections de relocation : %d\n\n", resultat->nb_sections);
    
    for (int i = 0; i < resultat->nb_sections; i++) {
        relocation_section_t *sec = &resultat->sections[i];
        
        printf("Relocation section '%s' at .text contains %zu entries:\n",
               sec->name, sec->count);
        
        if (sec->is_rela) {
            printf(" Offset     Info    Type         Sym.Value  Sym.Name + Addend\n");
        } else {
            printf(" Offset     Info    Type         Sym.Value  Sym.Name\n");
        }
        
        for (size_t j = 0; j < sec->count; j++) {
            relocation_entry_t *rel = &sec->entries[j];
            
            unsigned int sym_idx = ELF32_R_SYM(rel->r_info);
            unsigned int type = ELF32_R_TYPE(rel->r_info);
            
            const char *sym_name = "";
            Elf32_Word sym_value = 0;
            
            if (sym_idx < (unsigned)resultat_symboles->nb_symbols) {
                Elf32_Sym *sym = &resultat_symboles->symbols[sym_idx];
                sym_value = sym->st_value;
                
                if (ELF32_ST_TYPE(sym->st_info) == STT_SECTION) {
                    
                    sym_name = resultat_symboles->strtab + sym->st_name;
                } else {
                    sym_name = resultat_symboles->strtab + sym->st_name;
                }
            }
            
            if (sec->is_rela) {
                printf("%08x  %08x %-14s %08x   %s + %x\n",
                       rel->r_offset,
                       rel->r_info,
                       arm_reloc_type_name(type),
                       sym_value,
                       sym_name,
                       rel->r_addend);
            } else {
                printf("%08x  %08x %-14s %08x   %s\n",
                       rel->r_offset,
                       rel->r_info,
                       arm_reloc_type_name(type),
                       sym_value,
                       sym_name);
            }
        }
        printf("\n");
    }
    
    printf("========================================\n");
}
// lit un uint32 big-endian depuis le buffer
static uint32_t read_u32_be(unsigned char *data){
    if (is_big_endian()) {
        return *(uint32_t*)data;
    } else {
        return reverse_4(*(uint32_t*)data);
    }
}

static void write_u32_be(unsigned char *data, uint32_t value){
    if (is_big_endian()) {
        *(uint32_t*)data = value;
    } else {
        *(uint32_t*)data = reverse_4(value);
    }
}
// patch l'addend dans le contenu de section (pour SHT_REL)
static int patch_addend_section_rel(resultat_fusion_t *res_sections,
                                    relocation_entry_t *rel,
                                    int target_sec_new,
                                    unsigned int type,
                                    Elf32_Word delta_def){
    // trouver la section cible
    if (target_sec_new < 1 || target_sec_new > res_sections->nb_sections) {
        fprintf(stderr, "Erreur: target_sec_new %d hors bornes\n", target_sec_new);
        return -1;
    }
    
    section_fusionnee_t *sec = &res_sections->sections[target_sec_new - 1];
    
    
    if (rel->r_offset + 4 > sec->size) {
        fprintf(stderr, "Erreur: r_offset %x hors section (taille %x)\n",
                rel->r_offset, sec->size);
        return -1;
    }
    
    // lire l'addend implicite
    uint32_t word = read_u32_be(sec->data + rel->r_offset);
    
    // ajuster selon le type de relocation
    if (type == 2) { // R_ARM_ABS32 
        word += delta_def;
    } else if (type == 28 || type == 29 || type == 1) { // CALL/JUMP24/PC24
        // extraire imm24 signe
        int32_t imm24 = (int32_t)((word & 0x00FFFFFF) << 8) >> 8; 
        imm24 += delta_def / 4;
        
        word = (word & 0xFF000000) | (imm24 & 0x00FFFFFF);
    }
    
    
    write_u32_be(sec->data + rel->r_offset, word);
    
    return 0;
}
// fusionne les tables de relocations de 2 fichiers
int fusion_etape8(char *fichier1, char *fichier2,
                  resultat_fusion_t *resultat_sections,
                  resultat_symboles_t *resultat_symboles,
                  resultat_relocations_t *resultat_relocations){
    elf_contexte_t ctx1, ctx2;
    SymbolTable symtab1, symtab2;
    int ret = -1;
    
    memset(&symtab1, 0, sizeof(SymbolTable));
    memset(&symtab2, 0, sizeof(SymbolTable));
    memset(resultat_relocations, 0, sizeof(resultat_relocations_t));
    
    
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
        fprintf(stderr, "Erreur: impossible de lire symboles %s\n", fichier1);
        goto cleanup;
    }
    
    if (elf_lire_symboles(&ctx2, &symtab2) != 0) {
        fprintf(stderr, "Erreur: impossible de lire symboles %s\n", fichier2);
        goto cleanup;
    }
    
    // charger les tables de relocations
    elf_relocation_table_t relTab1, relTab2;
    memset(&relTab1, 0, sizeof(elf_relocation_table_t));
    memset(&relTab2, 0, sizeof(elf_relocation_table_t));
    
    if (elf_lire_relocations(&ctx1, &relTab1) != 0) {
        fprintf(stderr, "Erreur: impossible de lire relocations %s\n", fichier1);
        goto cleanup;
    }
    
    if (elf_lire_relocations(&ctx2, &relTab2) != 0) {
        fprintf(stderr, "Erreur: impossible de lire relocations %s\n", fichier2);
        elf_liberer_relocations(&relTab1);
        goto cleanup;
    }
    
    // fusionner par nom de section
    int max_sections = relTab1.section_count + relTab2.section_count;
    if (max_sections == 0) {
        // pas de relocations
        ret = 0;
        goto cleanup_rel;
    }
    
    resultat_relocations->sections = calloc(max_sections, sizeof(relocation_section_t));
    if (!resultat_relocations->sections) {
        fprintf(stderr, "Erreur: allocation mémoire\n");
        goto cleanup_rel;
    }
    
    int nb_sections = 0;
    
    // traiter fichier1
    for (size_t i = 0; i < relTab1.section_count; i++) {
        elf_relocation_section_t *relSec = &relTab1.sections[i];
        
        // calculer l'index de section cible apres fusion
        int target_old = relSec->target_sec;
        int target_new = -1;
        
        if (target_old < resultat_sections->map1_size) {
            target_new = resultat_sections->map1[target_old];
        }
        
        if (target_new < 0) {
            fprintf(stderr, "Warning: section relocation %s cible section non mappée\n",
                    relSec->name);
            continue;
        }
        
        
        int existing_idx = -1;
        for (int j = 0; j < nb_sections; j++) {
            if (strcmp(resultat_relocations->sections[j].name, relSec->name) == 0) {
                existing_idx = j;
                break;
            }
        }
        
        relocation_section_t *outSec;
        if (existing_idx >= 0) {
            
            outSec = &resultat_relocations->sections[existing_idx];
            if (outSec->is_rela != relSec->is_rela) {
                fprintf(stderr, "Erreur: incohérence is_rela pour %s\n", relSec->name);
                goto cleanup_rel;
            }
            if (outSec->target_sec_new != target_new) {
                fprintf(stderr, "Erreur: incohérence target_sec pour %s\n", relSec->name);
                goto cleanup_rel;
            }
            
            
            size_t new_count = outSec->count + relSec->count;
            relocation_entry_t *new_entries = realloc(outSec->entries,
                                                      new_count * sizeof(relocation_entry_t));
            if (!new_entries) {
                fprintf(stderr, "Erreur: allocation mémoire\n");
                goto cleanup_rel;
            }
            outSec->entries = new_entries;
        } else {
            
            outSec = &resultat_relocations->sections[nb_sections];
            outSec->name = strdup(relSec->name);
            outSec->is_rela = relSec->is_rela;
            outSec->target_sec_new = target_new;
            outSec->count = 0;
            outSec->entries = malloc(relSec->count * sizeof(relocation_entry_t));
            if (!outSec->entries) {
                fprintf(stderr, "Erreur: allocation mémoire\n");
                goto cleanup_rel;
            }
            nb_sections++;
        }
        
        
        size_t old_count = outSec->count;
        for (size_t j = 0; j < relSec->count; j++) {
            elf_relocation_t *relIn = &relSec->rels[j];
            relocation_entry_t *relOut = &outSec->entries[old_count + j];
            
            // fichier1: pas de correction d'offset
            relOut->r_offset = relIn->r_offset;
            
            // renumeroter le symbole
            unsigned int oldSym = ELF32_R_SYM(relIn->r_info);
            unsigned int type = ELF32_R_TYPE(relIn->r_info);
            
            int newSym = -1;
            if (oldSym < (unsigned)resultat_symboles->sym_map1_size) {
                newSym = resultat_symboles->sym_map1[oldSym];
            }
            
            
            if (oldSym == 0) {
                newSym = 0;
            }
            
            if (newSym < 0 || newSym >= resultat_symboles->nb_symbols) {
                fprintf(stderr, "Erreur: symbole %u (fichier1) non mappé ou hors bornes (newSym=%d)\n", oldSym, newSym);
                goto cleanup_rel;
            }
            
            relOut->r_info = ELF32_R_INFO(newSym, type);
            relOut->r_addend = relIn->r_addend;
            
            
            if (oldSym < symtab1.size &&
                ELF32_ST_TYPE(symtab1.symbols[oldSym].st_info) == STT_SECTION) {
                
                
                
                (void)target_new; 
            }
        }
        outSec->count += relSec->count;
    }
    
    // traiter fichier2
    for (size_t i = 0; i < relTab2.section_count; i++) {
        elf_relocation_section_t *relSec = &relTab2.sections[i];
        
        // calculer l'index de section cible apres fusion
        int target_old = relSec->target_sec;
        int target_new = -1;
        
        if (target_old < resultat_sections->map2_size) {
            target_new = resultat_sections->map2[target_old];
        }
        
        if (target_new < 0) {
            fprintf(stderr, "Warning: section relocation %s cible section non mappée\n",
                    relSec->name);
            continue;
        }
        
        
        int existing_idx = -1;
        for (int j = 0; j < nb_sections; j++) {
            if (strcmp(resultat_relocations->sections[j].name, relSec->name) == 0) {
                existing_idx = j;
                break;
            }
        }
        
        relocation_section_t *outSec;
        if (existing_idx >= 0) {
            
            outSec = &resultat_relocations->sections[existing_idx];
            if (outSec->is_rela != relSec->is_rela) {
                fprintf(stderr, "Erreur: incohérence is_rela pour %s\n", relSec->name);
                goto cleanup_rel;
            }
            if (outSec->target_sec_new != target_new) {
                fprintf(stderr, "Erreur: incohérence target_sec pour %s\n", relSec->name);
                goto cleanup_rel;
            }
            
            
            size_t new_count = outSec->count + relSec->count;
            relocation_entry_t *new_entries = realloc(outSec->entries,
                                                      new_count * sizeof(relocation_entry_t));
            if (!new_entries) {
                fprintf(stderr, "Erreur: allocation mémoire\n");
                goto cleanup_rel;
            }
            outSec->entries = new_entries;
        } else {
            
            outSec = &resultat_relocations->sections[nb_sections];
            outSec->name = strdup(relSec->name);
            outSec->is_rela = relSec->is_rela;
            outSec->target_sec_new = target_new;
            outSec->count = 0;
            outSec->entries = malloc(relSec->count * sizeof(relocation_entry_t));
            if (!outSec->entries) {
                fprintf(stderr, "Erreur: allocation mémoire\n");
                goto cleanup_rel;
            }
            nb_sections++;
        }
        
        
        size_t old_count = outSec->count;
        for (size_t j = 0; j < relSec->count; j++) {
            elf_relocation_t *relIn = &relSec->rels[j];
            relocation_entry_t *relOut = &outSec->entries[old_count + j];
            
            // fichier2: corriger r_offset avec delta
            Elf32_Word delta_offset = 0;
            if (target_old < resultat_sections->delta2_size) {
                delta_offset = resultat_sections->delta2[target_old];
            }
            relOut->r_offset = relIn->r_offset + delta_offset;
            
            
            unsigned int oldSym = ELF32_R_SYM(relIn->r_info);
            unsigned int type = ELF32_R_TYPE(relIn->r_info);
            
            int newSym = -1;
            if (oldSym < (unsigned)resultat_symboles->sym_map2_size) {
                newSym = resultat_symboles->sym_map2[oldSym];
            }
            
            
            if (oldSym == 0) {
                newSym = 0;
            }
            
            if (newSym < 0 || newSym >= resultat_symboles->nb_symbols) {
                fprintf(stderr, "Erreur: symbole %u (fichier2) non mappé ou hors bornes (newSym=%d)\n", oldSym, newSym);
                goto cleanup_rel;
            }
            
            relOut->r_info = ELF32_R_INFO(newSym, type);
            relOut->r_addend = relIn->r_addend;
            
            // corriger addend pour symboles SECTION
            if (oldSym < symtab2.size &&
                ELF32_ST_TYPE(symtab2.symbols[oldSym].st_info) == STT_SECTION) {
                
                Elf32_Half def_old_shndx = symtab2.symbols[oldSym].st_shndx;
                Elf32_Word delta_def = 0;
                
                
                if (def_old_shndx < resultat_sections->delta2_size) {
                    delta_def = resultat_sections->delta2[def_old_shndx];
                }
                
                if (outSec->is_rela) {
                    
                    if (type == 2) { 
                        relOut->r_addend += delta_def;
                    } else if (type == 28 || type == 29 || type == 1) {
                        
                        relOut->r_addend += delta_def / 4;
                    }
                } else {
                    
                    if (patch_addend_section_rel(resultat_sections, relOut,
                                                 target_new, type, delta_def) != 0) {
                        goto cleanup_rel;
                    }
                }
            }
        }
        outSec->count += relSec->count;
    }
    
    resultat_relocations->nb_sections = nb_sections;
    ret = 0;
    
cleanup_rel:
    elf_liberer_relocations(&relTab1);
    elf_liberer_relocations(&relTab2);
    
cleanup:
    elf_liberer_symboles(&symtab1);
    elf_liberer_symboles(&symtab2);
    elf_fermer(&ctx1);
    elf_fermer(&ctx2);
    
    if (ret != 0) {
        liberer_resultat_relocations(resultat_relocations);
    }
    
    return ret;
}
