#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

#include "elf_fusion_writer.h"
#include "elf_fusion_internal.h"
#include "elf_lecture.h"
#include "util.h"

typedef struct {
    const char *name;
    Elf32_Word name_off;   
    Elf32_Word type;
    Elf32_Word flags;
    Elf32_Word addr;
    Elf32_Word offset;
    Elf32_Word size;
    Elf32_Word link;
    Elf32_Word info;
    Elf32_Word addralign;
    Elf32_Word entsize;
    const unsigned char *data;
    int is_nobits;
} out_section_t;





static Elf32_Word align_to(Elf32_Word off, Elf32_Word align){
    if (align == 0)
        align = 1;

    return (off + align - 1) & ~(align - 1);
}


static Elf32_Word add_name(char **tab, size_t *size, const char *name){
    if (!name)
        name = "";
    size_t len = strlen(name) + 1;
    Elf32_Word off = (Elf32_Word)(*size);

    char *new_tab = realloc(*tab, *size + len);

    if (!new_tab)
        return (Elf32_Word)-1;
    memcpy(new_tab + *size, name, len);
    *tab = new_tab;
    *size += len;
    return off;
}


static void fill_sym_be(Elf32_Sym *dst, const Elf32_Sym *src){

    *dst = *src;

    if (!is_big_endian()) {
        dst->st_name  = reverse_4(dst->st_name);
        dst->st_value = reverse_4(dst->st_value);
        dst->st_size  = reverse_4(dst->st_size);
        dst->st_shndx = reverse_2(dst->st_shndx);
    }
}

static void fill_rel_be(relocation_entry_t *src, unsigned char *out, int is_rela){
    Elf32_Word r_offset = src->r_offset;
    Elf32_Word r_info   = src->r_info;
    Elf32_Sword addend  = src->r_addend;
    
    if (!is_big_endian()) {
        r_offset = reverse_4(r_offset);
        r_info   = reverse_4(r_info);
        addend   = reverse_4((Elf32_Word)addend);
    }
    
    memcpy(out, &r_offset, sizeof(Elf32_Word));

    memcpy(out + 4, &r_info, sizeof(Elf32_Word));
    
    if (is_rela) {
        memcpy(out + 8, &addend, sizeof(Elf32_Sword));
    }
}

static int generer_fichier_elf(char *sortie,
                               char *fichier1,
                               char *fichier2,
                               resultat_fusion_t *res_sec,
                               resultat_symboles_t *res_sym,
                               resultat_relocations_t *res_rel){



    int n_fused = res_sec->nb_sections;
    int n_rel = res_rel->nb_sections;
    
    int total_sections = 1 + n_fused + 2 + n_rel + 3; 
    
    out_section_t *secs = calloc(total_sections, sizeof(out_section_t));
    if (!secs) {
        
        fprintf(stderr, "Erreur: allocation sections sortie\n");
        
        return -1;
    }
    
    
    char *shstr = malloc(1);
    size_t shstr_size = 1;
    if (!shstr) {
        free(secs);
        return -1;
    }
    shstr[0] = '\0';
    
    
    int idx_null = 0;
    int idx_fused_start = 1;
    int idx_extra_start = idx_fused_start + n_fused;  
    int idx_rel_start = idx_extra_start + 2;
    int idx_symtab = idx_rel_start + n_rel;
    int idx_strtab = idx_symtab + 1;
    int idx_shstrtab = idx_symtab + 2;
    
    
    secs[idx_null].name = "";
    secs[idx_null].name_off = 0;
    secs[idx_null].type = SHT_NULL;
    secs[idx_null].addralign = 0;
    secs[idx_null].entsize = 0;
    secs[idx_null].is_nobits = 0;
    
    
    for (int i = 0; i < n_fused; i++) {
        section_fusionnee_t *s = &res_sec->sections[i];
        out_section_t *out = &secs[idx_fused_start + i];
        out->name = s->nom;
        out->name_off = add_name(&shstr, &shstr_size, s->nom);
        out->type = s->type;
        out->flags = s->flags;
        out->addr = 0;
        out->size = s->size;
        out->link = 0;
        out->info = 0;
        out->addralign = s->addralign ? s->addralign : 1;
        out->entsize = 0;
        out->data = s->data;
        out->is_nobits = 0;  
    }
    
    
    elf_contexte_t ctx1, ctx2;
    int bss_added = 0, arm_attr_added = 0;
    
    if (elf_ouvrir(fichier1, &ctx1) == 0) {
        
        for (int i = 0; i < ctx1.entete.e_shnum && !bss_added; i++) {
            if (ctx1.sections[i].sh_type == SHT_NOBITS) {
                const char *nom = ctx1.noms_sections + ctx1.sections[i].sh_name;
                if (strcmp(nom, ".bss") == 0) {
                    out_section_t *out = &secs[idx_extra_start];
                    out->name = ".bss";
                    out->name_off = add_name(&shstr, &shstr_size, ".bss");
                    out->type = SHT_NOBITS;
                    out->flags = ctx1.sections[i].sh_flags;
                    out->addr = 0;
                    out->size = ctx1.sections[i].sh_size;
                    out->link = 0;
                    out->info = 0;
                    out->addralign = ctx1.sections[i].sh_addralign ? ctx1.sections[i].sh_addralign : 1;
                    out->entsize = 0;
                    out->data = NULL;
                    out->is_nobits = 1;
                    bss_added = 1;
                }
            }
        }
        
        
        for (int i = 0; i < ctx1.entete.e_shnum && !arm_attr_added; i++) {
            if (ctx1.sections[i].sh_type == SHT_ARM_ATTRIBUTES) {
                const char *nom = ctx1.noms_sections + ctx1.sections[i].sh_name;
                if (strcmp(nom, ".ARM.attributes") == 0) {
                    
                    unsigned char *data = malloc(ctx1.sections[i].sh_size);
                    if (data) {
                        fseek(ctx1.fichier, ctx1.sections[i].sh_offset, SEEK_SET);
                        if (fread(data, 1, ctx1.sections[i].sh_size, ctx1.fichier) == ctx1.sections[i].sh_size) {
                            out_section_t *out = &secs[idx_extra_start + 1];
                            out->name = ".ARM.attributes";
                            out->name_off = add_name(&shstr, &shstr_size, ".ARM.attributes");
                            out->type = SHT_ARM_ATTRIBUTES;
                            out->flags = 0;
                            out->addr = 0;
                            out->size = ctx1.sections[i].sh_size;
                            out->link = 0;
                            out->info = 0;
                            out->addralign = ctx1.sections[i].sh_addralign ? ctx1.sections[i].sh_addralign : 1;
                            out->entsize = 0;
                            out->data = data;
                            out->is_nobits = 0;
                            arm_attr_added = 1;
                        } else {
                            free(data);
                        }
                    }
                }
            }
        }
        
        elf_fermer(&ctx1);
    }
    
    
    if (!bss_added && elf_ouvrir(fichier2, &ctx2) == 0) {
        for (int i = 0; i < ctx2.entete.e_shnum; i++) {
            if (ctx2.sections[i].sh_type == SHT_NOBITS) {
                const char *nom = ctx2.noms_sections + ctx2.sections[i].sh_name;
                if (strcmp(nom, ".bss") == 0) {
                    out_section_t *out = &secs[idx_extra_start];
                    out->name = ".bss";
                    out->name_off = add_name(&shstr, &shstr_size, ".bss");
                    out->type = SHT_NOBITS;
                    out->flags = ctx2.sections[i].sh_flags;
                    out->addr = 0;
                    out->size = ctx2.sections[i].sh_size;
                    out->link = 0;
                    out->info = 0;
                    out->addralign = ctx2.sections[i].sh_addralign ? ctx2.sections[i].sh_addralign : 1;
                    out->entsize = 0;
                    out->data = NULL;
                    out->is_nobits = 1;
                    bss_added = 1;
                    break;
                }
            }
        }
        elf_fermer(&ctx2);
    }
    
    
    if (!arm_attr_added && elf_ouvrir(fichier2, &ctx2) == 0) {
        for (int i = 0; i < ctx2.entete.e_shnum; i++) {
            if (ctx2.sections[i].sh_type == SHT_ARM_ATTRIBUTES) {
                const char *nom = ctx2.noms_sections + ctx2.sections[i].sh_name;
                if (strcmp(nom, ".ARM.attributes") == 0) {
                    unsigned char *data = malloc(ctx2.sections[i].sh_size);
                    if (data) {
                        fseek(ctx2.fichier, ctx2.sections[i].sh_offset, SEEK_SET);
                        if (fread(data, 1, ctx2.sections[i].sh_size, ctx2.fichier) == ctx2.sections[i].sh_size) {
                            out_section_t *out = &secs[idx_extra_start + 1];
                            out->name = ".ARM.attributes";
                            out->name_off = add_name(&shstr, &shstr_size, ".ARM.attributes");
                            out->type = SHT_ARM_ATTRIBUTES;
                            out->flags = 0;
                            out->addr = 0;
                            out->size = ctx2.sections[i].sh_size;
                            out->link = 0;
                            out->info = 0;
                            out->addralign = ctx2.sections[i].sh_addralign ? ctx2.sections[i].sh_addralign : 1;
                            out->entsize = 0;
                            out->data = data;
                            out->is_nobits = 0;
                            arm_attr_added = 1;
                        } else {
                            free(data);
                        }
                    }
                    break;
                }
            }
        }
        elf_fermer(&ctx2);
    }
    
    
    if (!bss_added) {
        out_section_t *out = &secs[idx_extra_start];
        out->name = ".bss";
        out->name_off = add_name(&shstr, &shstr_size, ".bss");
        out->type = SHT_NOBITS;
        out->flags = SHF_WRITE | SHF_ALLOC;
        out->addr = 0;
        out->size = 0;
        out->link = 0;
        out->info = 0;
        out->addralign = 1;
        out->entsize = 0;
        out->data = NULL;
        out->is_nobits = 1;
    }
    
    if (!arm_attr_added) {
        out_section_t *out = &secs[idx_extra_start + 1];
        out->name = ".ARM.attributes";
        out->name_off = add_name(&shstr, &shstr_size, ".ARM.attributes");
        out->type = SHT_ARM_ATTRIBUTES;
        out->flags = 0;
        out->addr = 0;
        out->size = 0;
        out->link = 0;
        out->info = 0;
        out->addralign = 1;
        out->entsize = 0;
        out->data = NULL;
        out->is_nobits = 0;
    }
    
    
    size_t *rel_sizes = calloc(n_rel, sizeof(size_t));
    unsigned char **rel_data = calloc(n_rel, sizeof(unsigned char*));
    if ((n_rel && (!rel_sizes || !rel_data))) {
        free(secs);
        free(shstr);
        free(rel_sizes);
        free(rel_data);
        return -1;
    }
    
    for (int i = 0; i < n_rel; i++) {
        relocation_section_t *rsec = &res_rel->sections[i];
        out_section_t *out = &secs[idx_rel_start + i];
        
        size_t entsize = rsec->is_rela ? sizeof(Elf32_Rela) : sizeof(Elf32_Rel);
        size_t total_size = rsec->count * entsize;
        rel_sizes[i] = total_size;
        rel_data[i] = malloc(total_size);
        if (!rel_data[i]) {
            free(secs);
            free(shstr);
            for (int k = 0; k < i; k++) free(rel_data[k]);
            free(rel_data);
            free(rel_sizes);
            return -1;
        }
        
        for (size_t j = 0; j < rsec->count; j++) {
            fill_rel_be(&rsec->entries[j],
                        rel_data[i] + j * entsize,
                        rsec->is_rela);
        }
        
        out->name = rsec->name;
        out->name_off = add_name(&shstr, &shstr_size, rsec->name);
        out->type = rsec->is_rela ? SHT_RELA : SHT_REL;
        out->flags = 0;
        out->addr = 0;
        out->size = (Elf32_Word)total_size;
        out->link = idx_symtab;
        out->info = rsec->target_sec_new;
        out->addralign = 4;
        out->entsize = entsize;
        out->data = rel_data[i];
        out->is_nobits = 0;
    }
    
    
    size_t sym_size = res_sym->nb_symbols * sizeof(Elf32_Sym);
    Elf32_Sym *symbuf = malloc(sym_size);
    if (!symbuf) {
        free(secs);
        free(shstr);
        for (int i = 0; i < n_rel; i++) free(rel_data[i]);
        free(rel_data);
        free(rel_sizes);
        return -1;
    }
    for (int i = 0; i < res_sym->nb_symbols; i++) {
        fill_sym_be(&symbuf[i], &res_sym->symbols[i]);
    }
    int last_local = 0;
    for (int i = 0; i < res_sym->nb_symbols; i++) {
        if (ELF32_ST_BIND(res_sym->symbols[i].st_info) == STB_LOCAL)
            last_local = i;
    }
    
    out_section_t *symsec = &secs[idx_symtab];
    symsec->name = ".symtab";
    symsec->name_off = add_name(&shstr, &shstr_size, ".symtab");
    symsec->type = SHT_SYMTAB;
    symsec->flags = 0;
    symsec->addr = 0;
    symsec->size = sym_size;
    symsec->link = idx_strtab;
    symsec->info = last_local + 1;
    symsec->addralign = 4;
    symsec->entsize = sizeof(Elf32_Sym);
    symsec->data = (unsigned char*)symbuf;
    symsec->is_nobits = 0;
    
    
    out_section_t *strsec = &secs[idx_strtab];
    strsec->name = ".strtab";
    strsec->name_off = add_name(&shstr, &shstr_size, ".strtab");
    strsec->type = SHT_STRTAB;
    strsec->flags = 0;
    strsec->addr = 0;
    strsec->size = res_sym->strtab_size;
    strsec->link = 0;
    strsec->info = 0;
    strsec->addralign = 1;
    strsec->entsize = 0;
    strsec->data = (unsigned char*)res_sym->strtab;
    strsec->is_nobits = 0;
    
    
    out_section_t *shstrsec = &secs[idx_shstrtab];
    shstrsec->name = ".shstrtab";
    shstrsec->name_off = add_name(&shstr, &shstr_size, ".shstrtab");
    shstrsec->type = SHT_STRTAB;
    shstrsec->flags = 0;
    shstrsec->addr = 0;
    shstrsec->size = shstr_size;
    shstrsec->link = 0;
    shstrsec->info = 0;
    shstrsec->addralign = 1;
    shstrsec->entsize = 0;
    shstrsec->data = (unsigned char*)shstr;
    shstrsec->is_nobits = 0;
    
    
    Elf32_Word off = sizeof(Elf32_Ehdr);
    for (int i = 1; i < total_sections; i++) {
        out_section_t *s = &secs[i];
        off = align_to(off, s->addralign);
        s->offset = off;
        if (!s->is_nobits)
            off += s->size;
    }
    
    Elf32_Word shoff = align_to(off, 4);
    
    
    Elf32_Ehdr eh;
    memset(&eh, 0, sizeof(eh));
    eh.e_ident[EI_MAG0] = ELFMAG0;
    eh.e_ident[EI_MAG1] = ELFMAG1;
    eh.e_ident[EI_MAG2] = ELFMAG2;
    eh.e_ident[EI_MAG3] = ELFMAG3;
    eh.e_ident[EI_CLASS] = ELFCLASS32;
    eh.e_ident[EI_DATA] = ELFDATA2MSB;
    eh.e_ident[EI_VERSION] = EV_CURRENT;
    eh.e_ident[EI_OSABI] = ELFOSABI_NONE;
    eh.e_ident[EI_ABIVERSION] = 0;
    
    eh.e_type = ET_REL;
    eh.e_machine = EM_ARM;
    eh.e_version = EV_CURRENT;
    eh.e_entry = 0;
    eh.e_phoff = 0;
    eh.e_shoff = shoff;
    eh.e_flags = 0;
    eh.e_ehsize = sizeof(Elf32_Ehdr);
    eh.e_phentsize = 0;
    eh.e_phnum = 0;
    eh.e_shentsize = sizeof(Elf32_Shdr);
    eh.e_shnum = total_sections;
    eh.e_shstrndx = idx_shstrtab;
    
    
    FILE *f = fopen(sortie, "wb");
    if (!f) {
        fprintf(stderr, "Erreur: impossible de créer %s\n", sortie);
        free(secs);
        free(shstr);
        free(symbuf);
        for (int i = 0; i < n_rel; i++) free(rel_data[i]);
        free(rel_data);
        free(rel_sizes);
        return -1;
    }
    
    
    if (!is_big_endian()) {
        eh.e_type      = reverse_2(eh.e_type);
        eh.e_machine   = reverse_2(eh.e_machine);
        eh.e_version   = reverse_4(eh.e_version);
        eh.e_entry     = reverse_4(eh.e_entry);
        eh.e_phoff     = reverse_4(eh.e_phoff);
        eh.e_shoff     = reverse_4(eh.e_shoff);
        eh.e_flags     = reverse_4(eh.e_flags);
        eh.e_ehsize    = reverse_2(eh.e_ehsize);
        eh.e_phentsize = reverse_2(eh.e_phentsize);
        eh.e_phnum     = reverse_2(eh.e_phnum);
        eh.e_shentsize = reverse_2(eh.e_shentsize);
        eh.e_shnum     = reverse_2(eh.e_shnum);
        eh.e_shstrndx  = reverse_2(eh.e_shstrndx);
    }
    
    fwrite(&eh, sizeof(eh), 1, f);
    
    
    Elf32_Word pos = sizeof(Elf32_Ehdr);
    for (int i = 1; i < total_sections; i++) {
        out_section_t *s = &secs[i];
        Elf32_Word pad = (s->offset > pos) ? (s->offset - pos) : 0;
        if (pad > 0) {
            unsigned char zero = 0;
            for (Elf32_Word p = 0; p < pad; p++)
                fwrite(&zero, 1, 1, f);
            pos += pad;
        }
        if (!s->is_nobits && s->size > 0 && s->data) {
            fwrite(s->data, 1, s->size, f);
            pos += s->size;
        }
    }
    
    
    if (pos < shoff) {
        unsigned char zero = 0;
        for (Elf32_Word p = pos; p < shoff; p++)
            fwrite(&zero, 1, 1, f);
    }
    
    
    for (int i = 0; i < total_sections; i++) {
        out_section_t *s = &secs[i];
        Elf32_Shdr sh;
        memset(&sh, 0, sizeof(sh));
        sh.sh_name = s->name_off;
        sh.sh_type = s->type;
        sh.sh_flags = s->flags;
        sh.sh_addr = s->addr;
        sh.sh_offset = s->offset;
        sh.sh_size = s->size;
        sh.sh_link = s->link;
        sh.sh_info = s->info;
        sh.sh_addralign = s->addralign ? s->addralign : 1;
        sh.sh_entsize = s->entsize;
        
        if (!is_big_endian()) {
            sh.sh_name      = reverse_4(sh.sh_name);
            sh.sh_type      = reverse_4(sh.sh_type);
            sh.sh_flags     = reverse_4(sh.sh_flags);
            sh.sh_addr      = reverse_4(sh.sh_addr);
            sh.sh_offset    = reverse_4(sh.sh_offset);
            sh.sh_size      = reverse_4(sh.sh_size);
            sh.sh_link      = reverse_4(sh.sh_link);
            sh.sh_info      = reverse_4(sh.sh_info);
            sh.sh_addralign = reverse_4(sh.sh_addralign);
            sh.sh_entsize   = reverse_4(sh.sh_entsize);
        }
        fwrite(&sh, sizeof(sh), 1, f);
    }
    
    fclose(f);
    
    
    free(secs);
    free(shstr);
    free(symbuf);
    for (int i = 0; i < n_rel; i++) free(rel_data[i]);
    free(rel_data);
    free(rel_sizes);
    
    return 0;
}


int fusion_etape9(char *fichier1, char *fichier2, char *fichier_sortie){
    resultat_fusion_t res_sec;
    resultat_symboles_t res_sym;
    resultat_relocations_t res_rel;
    int ret = -1;
    
    if (fusion_etape6(fichier1, fichier2, &res_sec) != 0) {
        fprintf(stderr, "Erreur: fusion sections (étape 6)\n");
        return -1;
    }
    
    if (fusion_etape7(fichier1, fichier2, &res_sec, &res_sym) != 0) {
        fprintf(stderr, "Erreur: fusion symboles (étape 7)\n");
        liberer_resultat_fusion(&res_sec);
        return -1;
    }
    
    if (fusion_etape8(fichier1, fichier2, &res_sec, &res_sym, &res_rel) != 0) {
        fprintf(stderr, "Erreur: fusion relocations (étape 8)\n");
        liberer_resultat_symboles(&res_sym);
        liberer_resultat_fusion(&res_sec);
        return -1;
    }
    
    ret = generer_fichier_elf(fichier_sortie, fichier1, fichier2, &res_sec, &res_sym, &res_rel);
    
    liberer_resultat_relocations(&res_rel);
    liberer_resultat_symboles(&res_sym);
    liberer_resultat_fusion(&res_sec);
    
    return ret;
}
