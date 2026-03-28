#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include "elf_affichage_sections.h"

#define SECTION_FLAGS_MAX 10

static char *sh_type_str(Elf32_Word t) {
    switch (t) {
        case SHT_NULL:     return "NULL";
        case SHT_PROGBITS: return "PROGBITS";
        case SHT_SYMTAB:   return "SYMTAB";
        case SHT_STRTAB:   return "STRTAB";
        case SHT_RELA:     return "RELA";
        case SHT_HASH:     return "HASH";
        case SHT_DYNAMIC:  return "DYNAMIC";
        case SHT_NOTE:     return "NOTE";
        case SHT_NOBITS:   return "NOBITS";
        case SHT_REL:      return "REL";
        case SHT_SHLIB:    return "SHLIB";
        case SHT_DYNSYM:   return "DYNSYM";
        case SHT_ARM_ATTRIBUTES: return "ARM_ATTRIBUTES";
        default:           return "UNKNOWN";
    }
}


static void section_flags_str(Elf32_Word flags, char *out) {
    int i = 0;

    if (flags & SHF_WRITE)        out[i++] = 'W';
    if (flags & SHF_ALLOC)        out[i++] = 'A';
    if (flags & SHF_EXECINSTR)    out[i++] = 'X';
    if (flags & SHF_MERGE)        out[i++] = 'M';
    if (flags & SHF_STRINGS)      out[i++] = 'S';
    if (flags & SHF_INFO_LINK)    out[i++] = 'I';
    if (flags & SHF_LINK_ORDER)   out[i++] = 'L';
    if (flags & SHF_GROUP)        out[i++] = 'G';
    if (flags & SHF_TLS)          out[i++] = 'T';

    out[i] = '\0';
}

void elf_afficher_sections(elf_contexte_t *c)
{
    printf("There are %u section headers, starting at offset 0x%x:\n\n",
           c->entete.e_shnum, c->entete.e_shoff);

    printf("Section Headers:\n");
    printf("  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al\n");

    for (int i = 0; i < c->entete.e_shnum; i++) {
        Elf32_Shdr *s = &c->sections[i];
        char *name = c->noms_sections + s->sh_name;

        char flags[SECTION_FLAGS_MAX];
        section_flags_str(s->sh_flags, flags);

        printf("  [%2d] %-17.17s %-14s %08x %06x %06x %02x %-3s %2u %3u %2u\n",
               i,
               name,
               sh_type_str(s->sh_type),
               s->sh_addr,
               s->sh_offset,
               s->sh_size,
               s->sh_entsize,
               flags,
               s->sh_link,
               s->sh_info,
               s->sh_addralign);
    }

    printf("Key to Flags:\n");
    printf("  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),\n");
    printf("  L (link order), O (extra OS processing required), G (group), T (TLS),\n");
    printf("  C (compressed), x (unknown), o (OS specific), E (exclude),\n");
    printf("  D (mbind), y (purecode), p (processor specific)\n");
}
