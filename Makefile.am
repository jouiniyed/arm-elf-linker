ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS  = -Wall -Werror -g
AM_CFLAGS += -D DEBUG
AM_CFLAGS += -D WARNING
# Uncomment if performance when running with -DDEBUG is an issue
# Warning, if uncommented, issuing calls to debug functions during options
# parsing might result in debug flag incorrectly set to 0 for some files
#AM_CFLAGS+=-D CACHE_DEBUG_FLAG

AM_CPPFLAGS = -I$(top_srcdir)/include

SUBDIRS =
SUBDIRS += .

bin_PROGRAMS =

bin_PROGRAMS += mon_readelf
bin_PROGRAMS += elf_fusion

mon_readelf_SOURCES = \
	mon_readelf_main.c \
	src/elf_lecture.c \
	src/elf_affichage_entete.c \
	src/elf_affichage_sections.c \
	src/elf_affichage_contenu.c \
	src/elf_affichage_symboles.c \
	src/elf_affichage_relocations.c \
	util.c \
	debug.c

elf_fusion_SOURCES = \
	elf_fusion_main.c \
	src/elf_fusion.c \
	src/elf_fusion_sections.c \
	src/elf_fusion_symbols.c \
	src/elf_fusion_relocations.c \
	src/elf_fusion_writer.c \
	src/elf_lecture.c \
	src/elf_affichage_symboles.c \
	src/elf_affichage_relocations.c \
	util.c \
	debug.c

EXTRA_DIST = .gitignore

EXTRA_DIST +=
