#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "/workspaces/self-projects/project1/nasm_clone/include/elf64.h"
#include "/workspaces/self-projects/project1/nasm_clone/include/encoder.h"
#include "/workspaces/self-projects/project1/nasm_clone/include/symtab.h"

void write_elf64_object(const char *filename, Encoded_Program *prog, Symbol_Table *st){
    FILE *f = fopen(filename, "wb");
    if(!f){
        perror("Failed to open output file!");
        return;
    }

    const char shstrtab_data[] = "\0.text\0.data\0.bss\0.rela.text.\0.symtab\0.strtab\0.shstrtab";
    size_t shstrtab_len = sizeof(shstrtab_data);

    char strtab_data[512] = "\0";
    size_t strtab_len = 1;

    Elf64_Sym syms[16];
    memset(syms, 0, sizeof(syms));
    int sym_count = 1;

    for(Symbol *s = st->head; s != NULL; s = s->next){
        if(sym_count >= 16) break;
        uint32_t name_offset = (uint32_t)strtab_len;
        strcpy(&strtab_data[strtab_len], s->name);
        strtab_len += strlen(s->name) + 1;

        syms[sym_count].st_name = name_offset;
        syms[sym_count].st_info = ELF64_ST_INFO(s->is_global ? STB_GLOBAL : STB_LOCAL, STB_NOTYPE);
        syms[sym_count].st_other = 0;
        syms[sym_count].st_shndx = s->section_idx;
        syms[sym_count].st_value = s->offset;
        syms[sym_count].st_size = 0;
        sym_count++;
    }

    Elf64_Shdr shdrs[8];
    memset(shdrs, 0, sizeof(shdrs));

    uint64_t offset = sizeof(Elf64_Ehdr);

    // .text section
    shdrs[1].sh_name = 1;
    shdrs[1].sh_type = SHT_PROGBITS;
    shdrs[1].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    shdrs[1].sh_offset = offset;
    shdrs[1].sh_size = prog->text_len;
    shdrs[1].sh_addralign = 16;
    offset += prog->text_len;

    // .data section
    shdrs[2].sh_name = 7;
    shdrs[2].sh_type = SHT_PROGBITS;
    shdrs[2].sh_flags = SHF_ALLOC | SHF_WRITE;
    shdrs[2].sh_offset = offset;
    shdrs[2].sh_size = prog->data_len;
    shdrs[2].sh_addralign = 8;
    offset += prog->data_len;

    // .bss section
    shdrs[3].sh_name = 13;
    shdrs[3].sh_type = SHT_NOBITS;
    shdrs[3].sh_flags = SHF_ALLOC | SHF_WRITE;
    shdrs[3].sh_offset = offset;
    shdrs[3].sh_size = prog->bss_len;
    shdrs[3].sh_addralign = 16;

    // .rela.text
    shdrs[4].sh_name = 18;
    shdrs[4].sh_type = SHT_RELA;
    shdrs[4].sh_offset = offset;
    shdrs[4].sh_size = prog->rela_count * sizeof(Elf64_Rela);
    shdrs[4].sh_link = 4;
    shdrs[4].sh_info = 1;
    shdrs[4].sh_addralign = 8;
    shdrs[4].sh_entsize = sizeof(Elf64_Rela);
    offset += shdrs[4].sh_size;

    // .symtab
    shdrs[5].sh_name = 29;
    shdrs[5].sh_type = SHT_SYMTAB;
    shdrs[5].sh_offset = offset;
    shdrs[5].sh_size = sym_count * sizeof(Elf64_Sym);
    shdrs[5].sh_link = 5;
    shdrs[5].sh_info = 1;
    shdrs[5].sh_addralign = 8;
    shdrs[5].sh_entsize = sizeof(Elf64_Sym);
    offset += shdrs[5].sh_size;

    // .strtab
    shdrs[6].sh_name = 37;
    shdrs[6].sh_type = SHT_STRTAB;
    shdrs[6].sh_offset = offset;
    shdrs[6].sh_size = strtab_len;
    shdrs[6].sh_addralign = 1;
    offset += strtab_len;

    // .shstrtab
    shdrs[7].sh_name = 45;
    shdrs[7].sh_type = SHT_STRTAB;
    shdrs[7].sh_offset = offset;
    shdrs[7].sh_size = shstrtab_len;
    shdrs[7].sh_addralign = 1;
    offset += shstrtab_len;


    // ELF File Header
    Elf64_Ehdr ehdr;
    memset(&ehdr, 0, sizeof(Elf64_Ehdr));
    ehdr.e_ident[0] = ELFMAG0;
    ehdr.e_ident[1] = ELFMAG1;
    ehdr.e_ident[2] = ELFMAG2;
    ehdr.e_ident[3] = ELFMAG3;
    ehdr.e_ident[4] = ELFCLASS64;
    ehdr.e_ident[5] = ELFDATA2LSB;
    ehdr.e_ident[6] = EV_CURRENT;
    ehdr.e_ident[7] = ELFOSABI_SYSV;
    ehdr.e_type = ET_REL;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_shoff = offset;
    ehdr.e_shentsize = sizeof(Elf64_Shdr);
    ehdr.e_shnum = 8;
    ehdr.e_shstrndx = 7;

    // SEQUENTIAL WRITE

    fwrite(&ehdr, 1, sizeof(Elf64_Ehdr), f);

    if(prog->text_len > 0) fwrite(prog->text_bytes, 1, prog->text_len, f);
    if(prog->data_len > 0) fwrite(prog->data_bytes, 1, prog->data_len, f);
    if(prog->rela_count > 0) fwrite(prog->relas, 1, shdrs[3].sh_size, f);

    fwrite(syms, 1, shdrs[5].sh_size, f);
    fwrite(strtab_data, 1, strtab_len, f);
    fwrite(shstrtab_data, 1, shstrtab_len, f);
    fwrite(shdrs, 1, sizeof(shdrs), f);

    fclose(f);
}