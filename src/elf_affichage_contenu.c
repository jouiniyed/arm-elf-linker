#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include "elf_affichage_contenu.h"

void elf_afficher_contenu_section(elf_contexte_t *c, char *nom_ou_indice)
{
    int num_section = -1;
    char *fin;
    long val = strtol(nom_ou_indice, &fin, 10);
    
    /* section par indice */
    if (*fin == '\0') {
        num_section = (int)val;
    } else {
        /* section par nom */
        for (int i = 0; i < c->entete.e_shnum; i++) {
            char *nom = c->noms_sections + c->sections[i].sh_name;
            if (strcmp(nom, nom_ou_indice) == 0) {
                num_section = i;
                break;
            }
        }
    }

    if (num_section < 0 || num_section >= c->entete.e_shnum) {
        printf("readelf: Warning: Section '%s' was not dumped because it does not exist\n",
               nom_ou_indice);
        return;
    }

    Elf32_Shdr sec = c->sections[num_section];

    if (sec.sh_type == SHT_NOBITS || sec.sh_size == 0) {
        printf("Section '%s' has no data to dump.\n",
               c->noms_sections + sec.sh_name);
        return;
    }

    char *contenu = malloc(sec.sh_size);
    if (!contenu)
        return;

    fseek(c->fichier, sec.sh_offset, SEEK_SET);
    
    if (fread(contenu, 1, sec.sh_size, c->fichier) != sec.sh_size) {
        perror("fread");
        free(contenu);
        return;
    }

    /* Verifier si cette section a des relocations */
    int has_reloc = 0;
    for (int i = 0; i < c->entete.e_shnum; i++) {
        Elf32_Shdr *sh = &c->sections[i];
        if ((sh->sh_type == SHT_REL || sh->sh_type == SHT_RELA) && 
            sh->sh_info == num_section) {
            has_reloc = 1;
            break;
        }
    }

    printf("Hex dump of section '%s':\n",c->noms_sections + sec.sh_name);
    
    if (has_reloc) {
        printf("  NOTE: This section has relocations against it, but these have NOT been applied to this dump.\n");
    }

    for (size_t i = 0; i < sec.sh_size; i += 16) {
        printf("  0x%08zx ", i);

        size_t j;
        for (j = 0; j < 16 && i + j < sec.sh_size; j++) {
            printf("%02x", (unsigned char)contenu[i + j]);
            if (j % 4 == 3)
                printf(" ");
        }

        if (j < 16) {
            for (size_t k = j; k < 16; k++) {
                printf("  ");
                if (k % 4 == 3)
                    printf(" ");
            }
        }

        printf(" ");
        for (j = 0; j < 16 && i + j < sec.sh_size; j++) {
            unsigned char cch = contenu[i + j];
            printf("%c", (cch >= 32 && cch <= 126) ? cch : '.');
        }
        printf("\n");
    }

    free(contenu);
}
