#ifndef ELF_FUSION_H
#define ELF_FUSION_H

#include <elf.h>

typedef struct {
    char *nom;
    Elf32_Word type;
    Elf32_Word flags;
    Elf32_Word addralign;
    Elf32_Word size;
    Elf32_Word size_file1;
    Elf32_Word size_file2;
    unsigned char *data;
    int shndx_file1;
    int shndx_file2;
} section_fusionnee_t;

typedef struct {
    section_fusionnee_t *sections;
    int nb_sections;
    
    int *map1;
    int map1_size;
    
    int *map2;
    int map2_size;
    
    Elf32_Word *delta2;
    int delta2_size;
} resultat_fusion_t;

typedef struct {
    Elf32_Sym *symbols;
    int nb_symbols;
    
    char *strtab;
    size_t strtab_size;
    
    int *sym_map1;
    int sym_map1_size;
    
    int *sym_map2;
    int sym_map2_size;
} resultat_symboles_t;

typedef struct {
    Elf32_Addr  r_offset;
    Elf32_Word  r_info;
    Elf32_Sword r_addend;
} relocation_entry_t;

typedef struct {
    char *name;
    int is_rela;
    int target_sec_new;
    size_t count;
    relocation_entry_t *entries;
} relocation_section_t;

typedef struct {
    relocation_section_t *sections;
    int nb_sections;
} resultat_relocations_t;

#include "elf_fusion_sections.h"
#include "elf_fusion_symbols.h"
#include "elf_fusion_relocations.h"
#include "elf_fusion_writer.h"

#endif
