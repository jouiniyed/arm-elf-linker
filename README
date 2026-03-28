# Projet Linker ELF ARM 32-bit

Ce projet implémente un analyseur et un éditeur de liens (linker) simplifié pour les fichiers objets ELF ciblant l'architecture ARM 32-bit. 

Il se compose de deux utilitaires principaux :
- `mon_readelf` : Outil reproduisant les fonctionnalités basiques de readelf (lecture de l'en-tête, sections, contenu, table des symboles, relocations).
- `elf_fusion` : Un éditeur de liens basique permettant de fusionner deux fichiers objets (tables des sections, symboles, et relocations) et de générer un binaire exécutable simple.

## Prérequis

Pour compiler et tester ce projet, vous devez disposer des outils suivants :
- GNU Make
- GCC (compiler natif pour la compilation des exécutables `mon_readelf` et `elf_fusion`)
- GNU Autotools (autoconf, automake)
- La chaîne de compilation croisée ARM (`arm-none-eabi-gcc`, `arm-none-eabi-as`, `arm-none-eabi-ld`, `arm-none-eabi-readelf`, `arm-none-eabi-objcopy`) pour exécuter les tests et générer les objets ARM de référence.

## Compilation

Le projet utilise les Autotools GNU. Pour générer ou re-générer les fichiers de compilation, ainsi que pour compiler le projet :

1. Générer le script `configure` (si le Makefile n'est pas déjà présent) :
   ```bash
   autoreconf -fiv
   ```

2. Exécuter le script de configuration :
   ```bash
   ./configure
   ```

3. Compiler avec `make` :
   ```bash
   make
   ```

Cette séquence produira les exécutables `mon_readelf` et `elf_fusion` à la racine du projet.

## Exécution des utilitaires

### 1- `mon_readelf`
Affiche le contenu d'un fichier objet ELF pour l'architecture ARM.

Usage:
```bash
./mon_readelf [options] <fichier-objet.o>
```
Options disponibles :
- `-h`, `--header`   : Affiche l'en-tête principal ELF
- `-S`, `--sections` : Affiche la table des sections
- `-x`, `--hex <nom/index>` : Affiche le contenu hexadécimal d'une section donnée
- `-s`, `--symbols`  : Affiche la table des symboles
- `-r`, `--relocs`   : Affiche l'ensemble des relocations contenues dans le fichier

### 2- `elf_fusion`
Fusionne deux fichiers *.o* cibles.

Usage:
```bash
./elf_fusion [options] <fichier-1.o> <fichier-2.o> [fichier_sortie.o]
```
Options de développement :
- `-6` : Imprime la fusion des tables de sections.
- `-7` : Imprime la fusion de la table des symboles.
- `-8` : Imprime la table des relocations corrigées.
- `-9` : Étape complète de Link, fusionne l'entièreté des structures et produit le fichier résultat dans `fichier_sortie.o`.

## Tests

Un ensemble de scripts de tests est fourni dans le dossier `tests/`. Les jeux d'essais pour les tests sont présents dans le sous-dossier `tests/fixtures/`.

Lancer le script bash sans argument exécutera les scenarii fournis et vérifiera le bon comportement.

### Lancer les tests Readelf (Phase 1)
```bash
./tests/test_readelf.sh
```

### Lancer les tests de Fusion (Sections, Symboles, Relocations, Output)
```bash
./tests/test_section_merge.sh
./tests/test_symbol_merge.sh
./tests/test_relocation_merge.sh
./tests/test_elf_output.sh
```

Ces scripts utilisent `arm-none-eabi-ld` avec l'option `-r` (relocalisation) sur des fichiers d'assembleur contenus dans les `fixtures`  et comparent ensuite de façon automatique (`diff`) le résultat produit par le script par rapport au comportement d'un linker GNU de référence.
