#ifndef ELF_LECTURE_H
#define ELF_LECTURE_H

#include <stdio.h>
#include <elf.h>


typedef struct {
    FILE *fichier;
    Elf32_Ehdr entete;
    Elf32_Shdr *sections;
    char *noms_sections;
} elf_contexte_t;


int elf_ouvrir(char *nom_fichier, elf_contexte_t *contexte);
void elf_fermer(elf_contexte_t *contexte);

#endif
