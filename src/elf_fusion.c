#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include "elf_fusion.h"
#include "elf_fusion_internal.h"
#include "elf_lecture.h"

int est_section_progbits_valide(elf_contexte_t *ctx, int shndx){
    Elf32_Shdr *shdr = &ctx->sections[shndx];
    char *nom = ctx->noms_sections + shdr->sh_name;
    
    if (shdr->sh_type != SHT_PROGBITS)
        return 0;
    
    if (strstr(nom, ".debug") != NULL)
        return 0;
    
    return 1;
}

int trouver_section_par_nom(elf_contexte_t *ctx, const char *nom){
    for (int i = 0; i < ctx->entete.e_shnum; i++) {
        if (!est_section_progbits_valide(ctx, i))
            continue;
        
        char *nom_section = ctx->noms_sections + ctx->sections[i].sh_name;
        if (strcmp(nom_section, nom) == 0)
            return i;
    }
    return -1;
}

unsigned char *lire_contenu_section(elf_contexte_t *ctx, int shndx){
    Elf32_Shdr *shdr = &ctx->sections[shndx];
    
    if (shdr->sh_size == 0)
        return NULL;
    
    unsigned char *data = malloc(shdr->sh_size);
    if (!data)
        return NULL;
    
    fseek(ctx->fichier, shdr->sh_offset, SEEK_SET);
    if (fread(data, 1, shdr->sh_size, ctx->fichier) != shdr->sh_size) {
        free(data);
        return NULL;
    }
    
    return data;
}

unsigned char *concatener_sections(unsigned char *data1, Elf32_Word size1,
                                   unsigned char *data2, Elf32_Word size2){
    Elf32_Word total = size1 + size2;
    
    if (total == 0)
        return NULL;
    
    unsigned char *result = malloc(total);
    if (!result)
        return NULL;
    
    if (size1 > 0 && data1)
        memcpy(result, data1, size1);
    
    if (size2 > 0 && data2)
        memcpy(result + size1, data2, size2);
    
    return result;
}
