#ifndef ELF_AFFICHAGE_SYMBOLES_H
#define ELF_AFFICHAGE_SYMBOLES_H

#include <elf.h>
#include "elf_lecture.h"

typedef struct {
    Elf32_Sym *symbols;
    size_t size;
    char *strtab;
} SymbolTable;

int  elf_lire_symboles(elf_contexte_t *contexte, SymbolTable *symtab);
void elf_afficher_symboles(const elf_contexte_t *contexte,
                           const SymbolTable *symtab);
void elf_liberer_symboles(SymbolTable *symtab);

#endif
