#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include "elf_fusion_sections.h"
#include "elf_fusion_internal.h"
#include "elf_lecture.h"

// affiche le dump hexa d'une section
static void afficher_contenu_section_fusionnee(unsigned char *data, Elf32_Word size){
    if (size == 0) {
        printf("    (vide)\n");
        return;
    }
    
    for (Elf32_Word i = 0; i < size; i += 16) {
        printf("  0x%06x ", i);
        
        for (Elf32_Word j = 0; j < 16; j += 4) {
            if (i + j < size && i + j + 3 < size) {
                printf("%02x%02x%02x%02x ", 
                       data[i+j], data[i+j+1], data[i+j+2], data[i+j+3]);
            } else if (i + j < size) {
                for (Elf32_Word k = j; k < 16 && i + k < size; k++)
                    printf("%02x", data[i+k]);
                break;
            }
        }
        
        printf("\n");
    }
}

// fusionne les sections PROGBITS de 2 fichiers
int fusion_etape6(char *fichier1, char *fichier2, resultat_fusion_t *resultat){
    elf_contexte_t ctx1, ctx2;
    int ret = -1;
    
    if (elf_ouvrir(fichier1, &ctx1) != 0) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", fichier1);
        return -1;
    }
    
    if (elf_ouvrir(fichier2, &ctx2) != 0) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", fichier2);
        elf_fermer(&ctx1);
        return -1;
    }
    
    memset(resultat, 0, sizeof(resultat_fusion_t));
    
    resultat->map1_size = ctx1.entete.e_shnum;
    resultat->map2_size = ctx2.entete.e_shnum;
    resultat->delta2_size = ctx2.entete.e_shnum;
    resultat->map1 = malloc(resultat->map1_size * sizeof(int));
    resultat->map2 = malloc(resultat->map2_size * sizeof(int));
    resultat->delta2 = malloc(resultat->delta2_size * sizeof(Elf32_Word));
    
    if (!resultat->map1 || !resultat->map2 || !resultat->delta2) {
        fprintf(stderr, "Erreur: allocation mémoire\n");
        goto cleanup;
    }
    
    // init les maps a -1
    for (int i = 0; i < resultat->map1_size; i++)
        resultat->map1[i] = -1;
    
    for (int i = 0; i < resultat->map2_size; i++) {
        resultat->map2[i] = -1;
        resultat->delta2[i] = 0;
    }
    
    // marquer quelles sections de fichier2 sont deja traitees
    int *used2 = calloc(ctx2.entete.e_shnum, sizeof(int));
    if (!used2) {
        fprintf(stderr, "Erreur: allocation mémoire\n");
        goto cleanup;
    }
    
    int nb_max = ctx1.entete.e_shnum + ctx2.entete.e_shnum;
    section_fusionnee_t *sections = calloc(nb_max, sizeof(section_fusionnee_t));
    if (!sections) {
        fprintf(stderr, "Erreur: allocation mémoire\n");
        free(used2);
        goto cleanup;
    }
    
    int nb_sections = 0;
    int comment_traite = 0;
    
    // parcourir fichier1 et fusionner avec fichier2 si meme nom
    for (int i = 0; i < ctx1.entete.e_shnum; i++) {
        if (!est_section_progbits_valide(&ctx1, i))
            continue;
        
        char *nom1 = ctx1.noms_sections + ctx1.sections[i].sh_name;
        Elf32_Shdr *shdr1 = &ctx1.sections[i];
        
        if (strcmp(nom1, ".comment") == 0 && comment_traite)
            continue;
        
        int shndx2 = trouver_section_par_nom(&ctx2, nom1);
        
        section_fusionnee_t *sec = &sections[nb_sections];
        sec->nom = strdup(nom1);
        sec->type = SHT_PROGBITS;
        sec->flags = shdr1->sh_flags;
        sec->shndx_file1 = i;
        sec->addralign = shdr1->sh_addralign ? shdr1->sh_addralign : 1;
        
        resultat->map1[i] = nb_sections + 1;
        
        unsigned char *data1 = lire_contenu_section(&ctx1, i);
        unsigned char *data2 = NULL;
        
        if (shndx2 >= 0) {
            Elf32_Shdr *shdr2 = &ctx2.sections[shndx2];
            data2 = lire_contenu_section(&ctx2, shndx2);
            
            // .comment est special: on garde que fichier1
            if (strcmp(nom1, ".comment") == 0) {
                sec->size = shdr1->sh_size;
                sec->size_file1 = shdr1->sh_size;
                sec->size_file2 = 0;
                sec->data = data1;
                if (data2) free(data2);
                
                resultat->map2[shndx2] = nb_sections + 1;
                resultat->delta2[shndx2] = 0;
                
                comment_traite = 1;
            } else {
                // concatener fichier1 || fichier2
                sec->size = shdr1->sh_size + shdr2->sh_size;
                sec->size_file1 = shdr1->sh_size;
                sec->size_file2 = shdr2->sh_size;
                sec->data = concatener_sections(data1, shdr1->sh_size, 
                                                data2, shdr2->sh_size);
                
                if (data1) free(data1);
                if (data2) free(data2);
                
                resultat->map2[shndx2] = nb_sections + 1;
                resultat->delta2[shndx2] = shdr1->sh_size;
            }
            
            sec->shndx_file2 = shndx2;
            used2[shndx2] = 1;
        } else {
            sec->size = shdr1->sh_size;
            sec->size_file1 = shdr1->sh_size;
            sec->size_file2 = 0;
            sec->data = data1;
            sec->shndx_file2 = -1;
        }
        
        nb_sections++;
    }
    
    // ajouter les sections de fichier2 qui n'ont pas ete fusionnees
    for (int i = 0; i < ctx2.entete.e_shnum; i++) {
        if (!est_section_progbits_valide(&ctx2, i))
            continue;
        
        if (used2[i])
            continue;
        
        char *nom2 = ctx2.noms_sections + ctx2.sections[i].sh_name;
        
        if (strcmp(nom2, ".comment") == 0 && comment_traite)
            continue;
        
        Elf32_Shdr *shdr2 = &ctx2.sections[i];
        
        section_fusionnee_t *sec = &sections[nb_sections];
        sec->nom = strdup(nom2);
        sec->type = SHT_PROGBITS;
        sec->flags = shdr2->sh_flags;
        sec->size = shdr2->sh_size;
        sec->size_file1 = 0;
        sec->size_file2 = shdr2->sh_size;
        sec->data = lire_contenu_section(&ctx2, i);
        sec->shndx_file1 = -1;
        sec->shndx_file2 = i;
        sec->addralign = shdr2->sh_addralign ? shdr2->sh_addralign : 1;
        
        resultat->map2[i] = nb_sections + 1;
        resultat->delta2[i] = 0;
        
        nb_sections++;
    }
    
    resultat->sections = sections;
    resultat->nb_sections = nb_sections;
    
    free(used2);
    ret = 0;

cleanup:
    elf_fermer(&ctx1);
    elf_fermer(&ctx2);
    
    if (ret != 0) {
        liberer_resultat_fusion(resultat);
    }
    
    return ret;
}

void liberer_resultat_fusion(resultat_fusion_t *resultat){
    if (resultat->sections) {
        for (int i = 0; i < resultat->nb_sections; i++) {
            if (resultat->sections[i].nom)
                free(resultat->sections[i].nom);
            if (resultat->sections[i].data)
                free(resultat->sections[i].data);
        }
        free(resultat->sections);
    }
    
    if (resultat->map1)
        free(resultat->map1);
    
    if (resultat->map2)
        free(resultat->map2);
    
    if (resultat->delta2)
        free(resultat->delta2);
    
    memset(resultat, 0, sizeof(resultat_fusion_t));
}

void afficher_resultat_fusion(resultat_fusion_t *resultat){
    printf("Sections resultats (%d) :\n", resultat->nb_sections);
    for (int i = 0; i < resultat->nb_sections; i++) {
        section_fusionnee_t *sec = &resultat->sections[i];
        
        if (sec->shndx_file1 >= 0 && sec->shndx_file2 >= 0) {
            printf("  [%d] %s : FUSIONNEE (%u + %u = %u octets)\n",
                   i, sec->nom, sec->size_file1, sec->size_file2, sec->size);
        } else if (sec->shndx_file1 >= 0) {
            printf("  [%d] %s : fichier1 seulement (%u octets)\n",
                   i, sec->nom, sec->size);
        } else {
            printf("  [%d] %s : fichier2 seulement (%u octets)\n",
                   i, sec->nom, sec->size);
        }
    }
    
    printf("\nRenumerotation fichier2 :\n");
    for (int i = 0; i < resultat->map2_size; i++) {
        if (resultat->map2[i] >= 0) {
            printf("  Section [%d] -> [%d]  (delta = %u octets)\n", 
                   i, resultat->map2[i], resultat->delta2[i]);
        }
    }
    
    printf("\nContenu concatene :\n");
    for (int i = 0; i < resultat->nb_sections; i++) {
        section_fusionnee_t *sec = &resultat->sections[i];
        
        if (sec->shndx_file1 >= 0 && sec->shndx_file2 >= 0 && sec->size > 0) {
            printf("\n--- Section %s ---\n", sec->nom);
            printf("Taille totale: %u octets (%u + %u)\n", 
                   sec->size, sec->size_file1, sec->size_file2);
            printf("Offset fichier2: %u (delta)\n\n", sec->size_file1);
            afficher_contenu_section_fusionnee(sec->data, sec->size);
        }
    }
    
    printf("========================================\n");
}
