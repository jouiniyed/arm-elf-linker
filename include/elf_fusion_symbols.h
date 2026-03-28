#ifndef ELF_FUSION_SYMBOLS_H
#define ELF_FUSION_SYMBOLS_H

#include "elf_fusion.h"
// Symbol table merging from two object files

int fusion_etape7(char *fichier1, char *fichier2,
                  resultat_fusion_t *resultat_sections,
                  resultat_symboles_t *resultat_symboles);
void liberer_resultat_symboles(resultat_symboles_t *resultat);
void afficher_resultat_symboles(resultat_symboles_t *resultat);

#endif
