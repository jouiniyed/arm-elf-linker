#ifndef ELF_FUSION_RELOCATIONS_H
#define ELF_FUSION_RELOCATIONS_H

#include "elf_fusion.h"
// Relocation table merging from two object files

int fusion_etape8(char *fichier1, char *fichier2,
                  resultat_fusion_t *resultat_sections,
                  resultat_symboles_t *resultat_symboles,
                  resultat_relocations_t *resultat_relocations);
void liberer_resultat_relocations(resultat_relocations_t *resultat);
void afficher_resultat_relocations(resultat_relocations_t *resultat,
                                   resultat_symboles_t *resultat_symboles);

#endif
