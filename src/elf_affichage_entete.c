#include <stdio.h>
#include <elf.h>
#include "elf_affichage_entete.h"

void elf_afficher_entete(elf_contexte_t *c)
{
    Elf32_Ehdr *e = &c->entete;

    printf("ELF Header:\n");

    printf("  Magic:   ");
    for (int i = 0; i < EI_NIDENT; i++)
        printf("%02x ", e->e_ident[i]);
    printf("\n");

    printf("  Class:                             ELF32\n");
    printf("  Data:                              2's complement, big endian\n");
    printf("  Version:                           %d (current)\n", e->e_ident[EI_VERSION]);

    printf("  OS/ABI:                            ");
    switch (e->e_ident[EI_OSABI]) {
        case ELFOSABI_SYSV:  printf("UNIX - System V\n"); break;
        case ELFOSABI_LINUX: printf("UNIX - Linux\n"); break;
        case ELFOSABI_ARM:   printf("ARM\n"); break;
        default:             printf("Unknown\n"); break;
    }

    printf("  ABI Version:                       %d\n", e->e_ident[EI_ABIVERSION]);

    printf("  Type:                              ");
    switch (e->e_type) {
        case ET_REL:  printf("REL (Relocatable file)\n"); break;
        case ET_EXEC: printf("EXEC (Executable file)\n"); break;
        case ET_DYN:  printf("DYN (Shared object)\n"); break;
        default:      printf("UNKNOWN\n"); break;
    }

    printf("  Machine:                           ARM\n");
    printf("  Version:                           0x%x\n", e->e_version);
    printf("  Entry point address:               0x%x\n", e->e_entry);
    printf("  Start of program headers:          %u (bytes into file)\n", e->e_phoff);
    printf("  Start of section headers:          %u (bytes into file)\n", e->e_shoff);
    printf("  Flags:                             0x%x", e->e_flags);
    if (e->e_flags & 0x05000000) {
        printf(", Version5 EABI");
    }
    printf("\n");
    printf("  Size of this header:               %u (bytes)\n", e->e_ehsize);
    printf("  Size of program headers:           %u (bytes)\n", e->e_phentsize);
    printf("  Number of program headers:         %u\n", e->e_phnum);
    printf("  Size of section headers:           %u (bytes)\n", e->e_shentsize);
    printf("  Number of section headers:         %u\n", e->e_shnum);
    printf("  Section header string table index: %u\n", e->e_shstrndx);
}
