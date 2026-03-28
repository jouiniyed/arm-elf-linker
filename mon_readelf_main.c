#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "elf_lecture.h"
#include "elf_affichage_entete.h"
#include "elf_affichage_sections.h"
#include "elf_affichage_contenu.h"
#include "elf_affichage_symboles.h"
#include "elf_affichage_relocations.h"

static void afficher_aide(char *nom)
{
    printf("Usage: %s [options] fichier\n", nom);
    printf("Options:\n");
    printf("  -h, --header           Afficher l'en-tete ELF\n");
    printf("  -S, --sections         Afficher la table des sections\n");
    printf("  -x, --hex <sec>        Afficher le contenu d'une section\n");
    printf("  -s, --symbols          Afficher la table des symboles\n");
    printf("  -r, --relocs           Afficher les tables de relocation\n");
    printf("      --help             Afficher cette aide\n");
}

int main(int argc, char *argv[])
{
    int opt;
    int option_header = 0;
    int option_sections = 0;
    int option_symbols = 0;
    int option_relocs = 0;
    char *option_hex = NULL;

    static struct option options_longues[] = {
        {"header",   no_argument,       0, 'h'},
        {"sections", no_argument,       0, 'S'},
        {"hex",      required_argument, 0, 'x'},
        {"symbols",  no_argument,       0, 's'},
        {"relocs",   no_argument,       0, 'r'},
        {"help",     no_argument,       0,  0 },
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "hSx:sr", options_longues, NULL)) != -1) {
        switch (opt) {
            case 'h': option_header = 1; break;
            case 'S': option_sections = 1; break;
            case 'x': option_hex = optarg; break;
            case 's': option_symbols = 1; break;
            case 'r': option_relocs = 1; break;
            case 0:   afficher_aide(argv[0]); return 0;
            default:  afficher_aide(argv[0]); return 1;
        }
    }

    if (optind >= argc) {
        afficher_aide(argv[0]);
        return 1;
    }

    elf_contexte_t contexte;
    if (elf_ouvrir(argv[optind], &contexte) != 0) {
        fprintf(stderr, "Erreur: fichier ELF invalide ou non supporte\n");
        return 1;
    }

    SymbolTable symtab;
    int symtab_loaded = 0;

    if (option_symbols || option_relocs) {
        if (elf_lire_symboles(&contexte, &symtab) != 0) {
            fprintf(stderr, "Erreur: lecture symtab\n");
            elf_fermer(&contexte);
            return 1;
        }
        symtab_loaded = 1;
    }

    if (option_header)
        elf_afficher_entete(&contexte);

    if (option_sections)
        elf_afficher_sections(&contexte);

    if (option_hex)
        elf_afficher_contenu_section(&contexte, option_hex);

    if (option_symbols)
        elf_afficher_symboles(&contexte, &symtab);

    if (option_relocs) {
        elf_relocation_table_t rel_table;
        if (elf_lire_relocations(&contexte, &rel_table) == 0) {
            elf_afficher_relocations(&contexte, &rel_table, &symtab);
            elf_liberer_relocations(&rel_table);
        }
    }

    if (symtab_loaded)
        elf_liberer_symboles(&symtab);

    elf_fermer(&contexte);
    return 0;
}
