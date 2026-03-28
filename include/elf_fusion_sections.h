#ifndef ELF_FUSION_SECTIONS_H
#define ELF_FUSION_SECTIONS_H

#include "elf_fusion.h"
// Section merging (PROGBITS concatenation from two object files)

int fusion_etape6(char *fichier1, char *fichier2, resultat_fusion_t *resultat);
void liberer_resultat_fusion(resultat_fusion_t *resultat);
void afficher_resultat_fusion(resultat_fusion_t *resultat);

#endif
