#ifndef ELF_FUSION_INTERNAL_H
#define ELF_FUSION_INTERNAL_H

#include <elf.h>
#include "elf_lecture.h"
#include "elf_fusion.h"

int est_section_progbits_valide(elf_contexte_t *ctx, int shndx);
int trouver_section_par_nom(elf_contexte_t *ctx, const char *nom);
unsigned char *lire_contenu_section(elf_contexte_t *ctx, int shndx);
unsigned char *concatener_sections(unsigned char *data1, Elf32_Word size1,
                                   unsigned char *data2, Elf32_Word size2);

#endif
