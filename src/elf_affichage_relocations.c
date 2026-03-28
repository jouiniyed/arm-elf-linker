#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elf_affichage_relocations.h"
#include "util.h"

static const char *arm_rel_type(unsigned int t)
{
    switch (t) {
        case 0:    return "R_ARM_NONE";
        case 1:    return "R_ARM_PC24";
        case 2:    return "R_ARM_ABS32";
        case 3:    return "R_ARM_REL32";
        case 28:   return "R_ARM_CALL";
        case 29:   return "R_ARM_JUMP24";
        case 40:   return "R_ARM_V4BX";
        default:   return "UNKNOWN";
    }
}

int elf_lire_relocations(elf_contexte_t *c, elf_relocation_table_t *table)
{
    table->section_count = 0;
    table->sections = NULL;

    for (int i = 0; i < c->entete.e_shnum; i++)
        if (c->sections[i].sh_type == SHT_REL ||
            c->sections[i].sh_type == SHT_RELA)
            table->section_count++;

    if (!table->section_count)
        return 0;

    table->sections = calloc(table->section_count, sizeof(*table->sections));
    if (!table->sections)
        return -1;

    size_t k = 0;
    for (int i = 0; i < c->entete.e_shnum; i++) {
        Elf32_Shdr *s = &c->sections[i];

        if (s->sh_type != SHT_REL && s->sh_type != SHT_RELA)
            continue;

        elf_relocation_section_t *sec = &table->sections[k++];

        sec->name       = c->noms_sections + s->sh_name;
        sec->is_rela    = (s->sh_type == SHT_RELA);
        sec->target_sec = s->sh_info;
        sec->count      = s->sh_size / s->sh_entsize;

        if (!sec->count)
            continue;

        sec->rels = malloc(sec->count * sizeof(*sec->rels));
        if (!sec->rels)
            return -1;

        fseek(c->fichier, s->sh_offset, SEEK_SET);

        for (size_t j = 0; j < sec->count; j++) {
            if (!sec->is_rela) {
                Elf32_Rel r;
                if (fread(&r, sizeof(r), 1, c->fichier) != 1)
                    return -1;
                sec->rels[j].r_offset = is_big_endian() ? r.r_offset : reverse_4(r.r_offset);
                sec->rels[j].r_info   = is_big_endian() ? r.r_info   : reverse_4(r.r_info);
                sec->rels[j].r_addend = 0;
            } else {
                Elf32_Rela r;
                if (fread(&r, sizeof(r), 1, c->fichier) != 1)
                    return -1;
                sec->rels[j].r_offset = is_big_endian() ? r.r_offset : reverse_4(r.r_offset);
                sec->rels[j].r_info   = is_big_endian() ? r.r_info   : reverse_4(r.r_info);
                sec->rels[j].r_addend = is_big_endian() ? r.r_addend : reverse_4(r.r_addend);
            }
        }
    }
    return 0;
}

void elf_afficher_relocations(const elf_contexte_t *c,
                              const elf_relocation_table_t *table,
                              const SymbolTable *symtab)
{
    if (table->section_count == 0) {
        printf("\nThere are no relocations in this file.\n");
        return;
    }

    for (size_t i = 0; i < table->section_count; i++) {
        const elf_relocation_section_t *sec = &table->sections[i];

        /* Find the offset of this relocation section */
        uint32_t sec_offset = 0;
        for (int k = 0; k < c->entete.e_shnum; k++) {
            const char *name = c->noms_sections + c->sections[k].sh_name;
            if (strcmp(name, sec->name) == 0) {
                sec_offset = c->sections[k].sh_offset;
                break;
            }
        }

        printf("\nRelocation section '%s' at offset 0x%x contains %zu entries:\n",
               sec->name, sec_offset, sec->count);

        printf(sec->is_rela ?
            " Offset     Info    Type            Sym.Value  Sym. Name + Addend\n" :
            " Offset     Info    Type            Sym.Value  Sym. Name\n");

        for (size_t j = 0; j < sec->count; j++) {
            Elf32_Word info = sec->rels[j].r_info;
            uint32_t symi = ELF32_R_SYM(info);
            uint32_t rel_type = ELF32_R_TYPE(info);

            const Elf32_Sym *sym = NULL;
            const char *name = "";
            uint32_t sym_value = 0;

            /* Get symbol information if valid */
            if (symi < symtab->size && symi > 0) {
                sym = &symtab->symbols[symi];
                sym_value = sym->st_value;
                
                if (ELF32_ST_TYPE(sym->st_info) == STT_SECTION &&
                    sym->st_shndx < c->entete.e_shnum)
                    name = c->noms_sections + c->sections[sym->st_shndx].sh_name;
                else
                    name = symtab->strtab + sym->st_name;
            }

            if (!sec->is_rela) {
                if (symi > 0 && sym)
                    printf("%08x  %08x %-16s %08x   %s\n",
                           sec->rels[j].r_offset,
                           info,
                           arm_rel_type(rel_type),
                           sym_value,
                           name);
                else
                    printf("%08x  %08x %-16s\n",
                           sec->rels[j].r_offset,
                           info,
                           arm_rel_type(rel_type));
            } else {
                if (symi > 0 && sym)
                    printf("%08x  %08x %-16s %08x   %s + %08x\n",
                           sec->rels[j].r_offset,
                           info,
                           arm_rel_type(rel_type),
                           sym_value,
                           name,
                           sec->rels[j].r_addend);
                else
                    printf("%08x  %08x %-16s + %08x\n",
                           sec->rels[j].r_offset,
                           info,
                           arm_rel_type(rel_type),
                           sec->rels[j].r_addend);
            }
        }
    }
}



void elf_liberer_relocations(elf_relocation_table_t *table)
{
    if (!table)
        return;

    for (size_t i = 0; i < table->section_count; i++)
        free(table->sections[i].rels);

    free(table->sections);
    table->sections = NULL;
    table->section_count = 0;
}

