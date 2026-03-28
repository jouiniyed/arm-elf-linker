#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "elf_fusion.h"

static void afficher_aide(char *nom)
{
    printf("Usage: %s [options] fichier1.o fichier2.o\n", nom);
    printf("Options:\n");
    printf("  -6, --step6            Fusionner uniquement les sections (étape 6)\n");
    printf("  -7, --step7            Fusionner uniquement les symboles (étape 7)\n");
    printf("  -8, --step8            Fusionner sections + symboles + relocations (étapes 6+7+8)\n");
    printf("  -9, --step9            Produire un fichier ELF relocatable (étape 9)\n");
    printf("      --help             Afficher cette aide\n");
    printf("\n");
    printf("Note: Une option (-6, -7, -8 ou -9) est obligatoire\n");
}

// programme principal pour fusionner 2 fichiers .o
int main(int argc, char *argv[])
{
    int opt;
    int option_step6 = 0;
    int option_step7 = 0;
    int option_step8 = 0;
    int option_step9 = 0;
    
    static struct option options_longues[] = {
        {"step6",  no_argument, 0, '6'},
        {"step7",  no_argument, 0, '7'},
        {"step8",  no_argument, 0, '8'},
        {"step9",  no_argument, 0, '9'},
        {"help",   no_argument, 0,  0 },
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "6789", options_longues, NULL)) != -1) {
        switch (opt) {
            case '6': option_step6 = 1; break;
            case '7': option_step7 = 1; break;
            case '8': option_step8 = 1; break;
            case '9': option_step9 = 1; break;
            case 0:   afficher_aide(argv[0]); return 0;
            default:  afficher_aide(argv[0]); return 1;
        }
    }
    
    // verifier qu'une option est donnee
    if (!option_step6 && !option_step7 && !option_step8 && !option_step9) {
        fprintf(stderr, "Erreur: option -6, -7, -8 ou -9 requise\n\n");
        afficher_aide(argv[0]);
        return 1;
    }
    
    // etape 9 ne se combine pas avec les autres
    if (option_step9 && (option_step6 || option_step7 || option_step8)) {
        fprintf(stderr, "Erreur: l'option -9 est exclusive\n\n");
        afficher_aide(argv[0]);
        return 1;
    }
    
    if (option_step9) {
        if (optind + 3 != argc) {
            fprintf(stderr, "Erreur: -9 requiert fichier1.o fichier2.o fichier_sortie.o\n\n");
            afficher_aide(argv[0]);
            return 1;
        }
    } else {
        if (optind + 2 != argc) {
            afficher_aide(argv[0]);
            return 1;
        }
    }
    
    char *fichier1 = argv[optind];
    char *fichier2 = argv[optind + 1];
    char *fichier_sortie = option_step9 ? argv[optind + 2] : NULL;
    
    resultat_fusion_t resultat_sections;
    resultat_symboles_t resultat_symboles;
    resultat_relocations_t resultat_relocations;
    int ret = 0;
    
    printf("Fusion de %s et %s...\n\n", fichier1, fichier2);
    
    // etape 6: fusion des sections
    if (option_step6) {
        printf("=== Étape 6 : Fusion des sections ===\n\n");
        if (fusion_etape6(fichier1, fichier2, &resultat_sections) != 0) {
            fprintf(stderr, "Erreur lors de la fusion des sections\n");
            return 1;
        }
        
        afficher_resultat_fusion(&resultat_sections);
    }
    
    // etape 7: fusion des symboles (necessite etape 6)
    if (option_step7) {
        if (!option_step6) {
            if (fusion_etape6(fichier1, fichier2, &resultat_sections) != 0) {
                fprintf(stderr, "Erreur lors de la fusion des sections (prérequis pour étape 7)\n");
                return 1;
            }
        }
        
        if (option_step6) {
            printf("\n=== Étape 7 : Fusion des symboles ===\n\n");
        } else {
            printf("=== Étape 7 : Fusion des symboles ===\n\n");
        }
        
        if (fusion_etape7(fichier1, fichier2, &resultat_sections, &resultat_symboles) != 0) {
            fprintf(stderr, "Erreur lors de la fusion des symboles\n");
            liberer_resultat_fusion(&resultat_sections);
            return 1;
        }
        
        afficher_resultat_symboles(&resultat_symboles);
        ret = 0;
    }
    
    // etape 8: fusion des relocations (necessite 6+7)
    if (option_step8) {
        printf("=== Étape 8 : Fusion des relocations (6+7+8) ===\n\n");
        
        if (fusion_etape6(fichier1, fichier2, &resultat_sections) != 0) {
            fprintf(stderr, "Erreur lors de la fusion des sections (étape 6)\n");
            return 1;
        }
        
        if (fusion_etape7(fichier1, fichier2, &resultat_sections, &resultat_symboles) != 0) {
            fprintf(stderr, "Erreur lors de la fusion des symboles (étape 7)\n");
            liberer_resultat_fusion(&resultat_sections);
            return 1;
        }
        
        if (fusion_etape8(fichier1, fichier2, &resultat_sections, 
                         &resultat_symboles, &resultat_relocations) != 0) {
            fprintf(stderr, "Erreur lors de la fusion des relocations (étape 8)\n");
            liberer_resultat_symboles(&resultat_symboles);
            liberer_resultat_fusion(&resultat_sections);
            return 1;
        }
        
        afficher_resultat_relocations(&resultat_relocations, &resultat_symboles);
        ret = 0;
    }
    
    // etape 9: generer le fichier ELF final
    if (option_step9) {
        printf("=== Étape 9 : Production du fichier ELF ===\n\n");
        if (fusion_etape9(fichier1, fichier2, fichier_sortie) != 0) {
            fprintf(stderr, "Erreur lors de la production du fichier ELF (étape 9)\n");
            return 1;
        }
        printf("Fichier généré : %s\n", fichier_sortie);
        ret = 0;
    }
    
    if (option_step8) {
        liberer_resultat_relocations(&resultat_relocations);
        liberer_resultat_symboles(&resultat_symboles);
        liberer_resultat_fusion(&resultat_sections);
    } else if (option_step7) {
        liberer_resultat_symboles(&resultat_symboles);
        liberer_resultat_fusion(&resultat_sections);
    } else if (option_step6) {
        liberer_resultat_fusion(&resultat_sections);
    }
    
    return ret;
}
