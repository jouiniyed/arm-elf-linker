#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include "elf_lecture.h"
#include "util.h"

/* Vérifie le format ELF32 big endian */
static int verifier_elf(Elf32_Ehdr *e)
{
    return (e->e_ident[EI_MAG0] == ELFMAG0 &&
            e->e_ident[EI_MAG1] == ELFMAG1 &&
            e->e_ident[EI_MAG2] == ELFMAG2 &&
            e->e_ident[EI_MAG3] == ELFMAG3 &&
            e->e_ident[EI_CLASS] == ELFCLASS32 &&
            e->e_ident[EI_DATA] == ELFDATA2MSB);
}

int elf_ouvrir(char *nom_fichier, elf_contexte_t *contexte)
{
    contexte->fichier = NULL;
    contexte->sections = NULL;
    contexte->noms_sections = NULL;

    contexte->fichier = fopen(nom_fichier, "rb");
    if (!contexte->fichier)
        return -1;

    if (fread(&contexte->entete, sizeof(Elf32_Ehdr), 1, contexte->fichier) != 1) {
        elf_fermer(contexte);
        return -1;
    }

    /* Vérification ELF32 big endian */
    if (!verifier_elf(&contexte->entete)) {
        elf_fermer(contexte);
        return -1;
    }

    /* Conversion endian: EN-TETE ELF  */
    if (!is_big_endian()) {
        contexte->entete.e_type      = reverse_2(contexte->entete.e_type);
        contexte->entete.e_machine   = reverse_2(contexte->entete.e_machine);
        contexte->entete.e_version   = reverse_4(contexte->entete.e_version);
        contexte->entete.e_entry     = reverse_4(contexte->entete.e_entry);
        contexte->entete.e_phoff     = reverse_4(contexte->entete.e_phoff);
        contexte->entete.e_shoff     = reverse_4(contexte->entete.e_shoff);
        contexte->entete.e_flags     = reverse_4(contexte->entete.e_flags);
        contexte->entete.e_ehsize    = reverse_2(contexte->entete.e_ehsize);
        contexte->entete.e_phentsize = reverse_2(contexte->entete.e_phentsize);
        contexte->entete.e_phnum     = reverse_2(contexte->entete.e_phnum);
        contexte->entete.e_shentsize = reverse_2(contexte->entete.e_shentsize);
        contexte->entete.e_shnum     = reverse_2(contexte->entete.e_shnum);
        contexte->entete.e_shstrndx  = reverse_2(contexte->entete.e_shstrndx);
    }


    /* Vérification architecture ARM */
    if (contexte->entete.e_machine != EM_ARM) {
        elf_fermer(contexte);
        return -1;
    }

    /* Lecture table des sections */
    contexte->sections = malloc(contexte->entete.e_shnum * sizeof(Elf32_Shdr));
    if (!contexte->sections) {
        elf_fermer(contexte);
        return -1;
    }

    fseek(contexte->fichier, contexte->entete.e_shoff, SEEK_SET);
    if (fread(contexte->sections, sizeof(Elf32_Shdr),contexte->entete.e_shnum, contexte->fichier) != contexte->entete.e_shnum) {
        elf_fermer(contexte);
        return -1;
    }

    /* Conversion endian: TABLE DES SECTIONS */
    if (!is_big_endian()) {
        for (int i = 0; i < contexte->entete.e_shnum; i++) {
            contexte->sections[i].sh_name      = reverse_4(contexte->sections[i].sh_name);
            contexte->sections[i].sh_type      = reverse_4(contexte->sections[i].sh_type);
            contexte->sections[i].sh_flags     = reverse_4(contexte->sections[i].sh_flags);
            contexte->sections[i].sh_addr      = reverse_4(contexte->sections[i].sh_addr);
            contexte->sections[i].sh_offset    = reverse_4(contexte->sections[i].sh_offset);
            contexte->sections[i].sh_size      = reverse_4(contexte->sections[i].sh_size);
            contexte->sections[i].sh_link      = reverse_4(contexte->sections[i].sh_link);
            contexte->sections[i].sh_info      = reverse_4(contexte->sections[i].sh_info);
            contexte->sections[i].sh_addralign = reverse_4(contexte->sections[i].sh_addralign);
            contexte->sections[i].sh_entsize   = reverse_4(contexte->sections[i].sh_entsize);
        }
    }

    /* Chargement de la table des noms de sections .shstrtab */
    Elf32_Shdr shstr = contexte->sections[contexte->entete.e_shstrndx];

    contexte->noms_sections = malloc(shstr.sh_size);
    if (!contexte->noms_sections) {
        elf_fermer(contexte);
        return -1;
    }

    fseek(contexte->fichier, shstr.sh_offset, SEEK_SET);
    if (fread(contexte->noms_sections, 1, shstr.sh_size, contexte->fichier) != shstr.sh_size) {
        elf_fermer(contexte);
        return -1;
    }

    return 0;
}

void elf_fermer(elf_contexte_t *contexte)
{
    if (contexte->noms_sections) {
        free(contexte->noms_sections);
        contexte->noms_sections = NULL;
    }

    if (contexte->sections) {
        free(contexte->sections);
        contexte->sections = NULL;
    }

    if (contexte->fichier) {
        fclose(contexte->fichier);
        contexte->fichier = NULL;
    }
}
