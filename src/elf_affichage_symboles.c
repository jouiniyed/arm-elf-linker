#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include "elf_affichage_symboles.h"
#include "util.h"

static const char *type_str[] = {
    "NOTYPE", "OBJECT", "FUNC", "SECTION", "FILE", "COMMON", "TLS"
};

static const char *bind_str[] = {
    "LOCAL", "GLOBAL", "WEAK"
};

static const char *vis_str[] = {
    "DEFAULT", "INTERNAL", "HIDDEN", "PROTECTED"
};

int elf_lire_symboles(elf_contexte_t *contexte, SymbolTable *symtab)
{
    int index = -1;

    for (int i = 0; i < contexte->entete.e_shnum; i++) {
        const char *name = contexte->noms_sections + contexte->sections[i].sh_name;
        if (strcmp(name, ".symtab") == 0) {
            index = i;
            break;
        }
    }

    if (index < 0)
        return -1;

    Elf32_Shdr *symsec = &contexte->sections[index];
    symtab->size = symsec->sh_size / sizeof(Elf32_Sym);

    symtab->symbols = malloc(symsec->sh_size);
    if (!symtab->symbols)
        return -1;

    fseek(contexte->fichier, symsec->sh_offset, SEEK_SET);
    if (fread(symtab->symbols, sizeof(Elf32_Sym),
              symtab->size, contexte->fichier) != symtab->size)
        return -1;

    if (!is_big_endian()) {
        for (size_t i = 0; i < symtab->size; i++) {
            symtab->symbols[i].st_name  = reverse_4(symtab->symbols[i].st_name);
            symtab->symbols[i].st_value = reverse_4(symtab->symbols[i].st_value);
            symtab->symbols[i].st_size  = reverse_4(symtab->symbols[i].st_size);
            symtab->symbols[i].st_shndx = reverse_2(symtab->symbols[i].st_shndx);
        }
    }

    Elf32_Shdr *strsec = &contexte->sections[symsec->sh_link];
    symtab->strtab = malloc(strsec->sh_size);
    if (!symtab->strtab)
        return -1;

    fseek(contexte->fichier, strsec->sh_offset, SEEK_SET);
    if (fread(symtab->strtab, 1,
              strsec->sh_size, contexte->fichier) != strsec->sh_size)
        return -1;

    return 0;
}

void elf_afficher_symboles(const elf_contexte_t *contexte,
                           const SymbolTable *symtab)
{
    printf("\nSymbol table '.symtab' contains %zu entries:\n", symtab->size);
    printf("   Num:    Value  Size Type    Bind   Vis       Ndx  Name\n");

    for (size_t i = 0; i < symtab->size; i++) {
        const Elf32_Sym *s = &symtab->symbols[i];

        unsigned type = ELF32_ST_TYPE(s->st_info);
        unsigned bind = ELF32_ST_BIND(s->st_info);
        unsigned vis  = s->st_other & 0x3;

        const char *name;
        if (contexte && type == STT_SECTION && s->st_shndx < contexte->entete.e_shnum)
            name = contexte->noms_sections +
                   contexte->sections[s->st_shndx].sh_name;
        else
            name = symtab->strtab + s->st_name;

        if (i == 0)
            name = "";

        char ndx_buf[8];
        const char *ndx;

        if (s->st_shndx == SHN_UNDEF)
            ndx = "UND";
        else if (s->st_shndx == SHN_ABS)
            ndx = "ABS";
        else if (s->st_shndx == SHN_COMMON)
            ndx = "COM";
        else {
            sprintf(ndx_buf, "%u", s->st_shndx);
            ndx = ndx_buf;
        }

        printf("  %4zu: %08x %5u %-7s %-6s %-8s %4s %s\n",
               i,
               s->st_value,
               s->st_size,
               (type < 7 ? type_str[type] : "UNKNOWN"),
               (bind < 3 ? bind_str[bind] : "UNKNOWN"),
               (vis  < 4 ? vis_str[vis]  : "UNKNOWN"),
               ndx,
               name);
    }
}

void elf_liberer_symboles(SymbolTable *symtab)
{
    if (!symtab)
        return;

    free(symtab->symbols);
    free(symtab->strtab);

    symtab->symbols = NULL;
    symtab->strtab  = NULL;
    symtab->size    = 0;
}
