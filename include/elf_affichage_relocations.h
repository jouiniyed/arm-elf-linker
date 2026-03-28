#ifndef ELF_AFFICHAGE_RELOCATIONS_H
#define ELF_AFFICHAGE_RELOCATIONS_H

#include <elf.h>
#include <stdint.h>
#include "elf_lecture.h"
#include "elf_affichage_symboles.h"

typedef struct {
    Elf32_Addr  r_offset;
    Elf32_Word  r_info;
    Elf32_Sword r_addend;
} elf_relocation_t;

typedef struct {
    const char *name;
    int is_rela;
    uint32_t target_sec;
    size_t count;
    elf_relocation_t *rels;
} elf_relocation_section_t;

typedef struct {
    size_t section_count;
    elf_relocation_section_t *sections;
} elf_relocation_table_t;

int  elf_lire_relocations(elf_contexte_t *c, elf_relocation_table_t *table);
void elf_afficher_relocations(const elf_contexte_t *c,
                              const elf_relocation_table_t *table,
                              const SymbolTable *symtab);
void elf_liberer_relocations(elf_relocation_table_t *table);

#endif

