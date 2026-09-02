#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "/workspaces/self-projects/project1/nasm_clone/include/elf64.h"
#include "/workspaces/self-projects/project1/nasm_clone/include/encoder.h"
#include "/workspaces/self-projects/project1/nasm_clone/include/symtab.h"

// Helper to pad file offset to an alignment boundary
static uint64_t align_file_offset(FILE *f, uint64_t current_offset, uint64_t align) {
    if (align <= 1) return current_offset;
    uint64_t aligned = (current_offset + align - 1) & ~(align - 1);
    uint64_t diff = aligned - current_offset;
    if (diff > 0) {
        static const uint8_t zeros[64] = {0};
        fwrite(zeros, 1, diff, f);
    }
    return aligned;
}

void write_elf64_object(const char *filename, Encoded_Program *prog, Symbol_Table *st) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("Failed to open output file!");
        return;
    }

    const char shstrtab_data[] = "\0.text\0.data\0.bss\0.rela.text\0.symtab\0.strtab\0.shstrtab";
    size_t shstrtab_len = sizeof(shstrtab_data);

    char strtab_data[512] = "\0";
    size_t strtab_len = 1;

    Elf64_Sym syms[32];
    memset(syms, 0, sizeof(syms));
    int sym_count = 1; // Index 0 is always STN_UNDEF

    for(Symbol *s = st->head; s != NULL; s = s->next) {
        if (sym_count >= 32) break;
        uint32_t name_offset = (uint32_t)strtab_len;
        strcpy(&strtab_data[strtab_len], s->name);
        strtab_len += strlen(s->name) + 1;

        syms[sym_count].st_name = name_offset;

        // --- REPLACE THE ELF64_ST_INFO LINE HERE ---
        uint8_t bind = s->is_global ? 1 : 0; // 1 = STB_GLOBAL, 0 = STB_LOCAL
        uint8_t type = 0;                    // 0 = STT_NOTYPE
        syms[sym_count].st_info = (uint8_t)((bind << 4) | (type & 0x0F));
        //

        syms[sym_count].st_other = 0;
        syms[sym_count].st_shndx = s->section_idx;
        syms[sym_count].st_value = s->offset;
        syms[sym_count].st_size = 0;
        sym_count++;
    }

    Elf64_Shdr shdrs[8];
    memset(shdrs, 0, sizeof(shdrs));

    uint64_t offset = sizeof(Elf64_Ehdr);

    // .text section (Index 1)
    shdrs[1].sh_name = 1;
    shdrs[1].sh_type = SHT_PROGBITS;
    shdrs[1].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    shdrs[1].sh_offset = offset;
    shdrs[1].sh_size = prog->text_len;
    shdrs[1].sh_addralign = 16;
    offset += prog->text_len;

    // .data section (Index 2)
    shdrs[2].sh_name = 7;
    shdrs[2].sh_type = SHT_PROGBITS;
    shdrs[2].sh_flags = SHF_ALLOC | SHF_WRITE;
    shdrs[2].sh_offset = offset;
    shdrs[2].sh_size = prog->data_len;
    shdrs[2].sh_addralign = 8;
    offset += prog->data_len;

    // .bss section (Index 3)
    shdrs[3].sh_name = 13;
    shdrs[3].sh_type = SHT_NOBITS;
    shdrs[3].sh_flags = SHF_ALLOC | SHF_WRITE;
    shdrs[3].sh_offset = offset;
    shdrs[3].sh_size = prog->bss_len;
    shdrs[3].sh_addralign = 16;

    // .rela.text (Index 4)
    if (prog->rela_count > 0) {
        offset = (offset + 7) & ~7; // align 8
        shdrs[4].sh_name = 18;
        shdrs[4].sh_type = SHT_RELA;
        shdrs[4].sh_offset = offset;
        shdrs[4].sh_size = prog->rela_count * sizeof(Elf64_Rela);
        shdrs[4].sh_link = 5;
        shdrs[4].sh_info = 1;
        shdrs[4].sh_addralign = 8;
        shdrs[4].sh_entsize = sizeof(Elf64_Rela);
        offset += shdrs[4].sh_size;
    }

    // .symtab (Index 5)
    offset = (offset + 7) & ~7; // align 8
    shdrs[5].sh_name = 29;
    shdrs[5].sh_type = SHT_SYMTAB;
    shdrs[5].sh_offset = offset;
    shdrs[5].sh_size = sym_count * sizeof(Elf64_Sym);
    shdrs[5].sh_link = 6;
    shdrs[5].sh_info = 1;
    shdrs[5].sh_addralign = 8;
    shdrs[5].sh_entsize = sizeof(Elf64_Sym);
    offset += shdrs[5].sh_size;

    // .strtab (Index 6)
    shdrs[6].sh_name = 37;
    shdrs[6].sh_type = SHT_STRTAB;
    shdrs[6].sh_offset = offset;
    shdrs[6].sh_size = strtab_len;
    shdrs[6].sh_addralign = 1;
    offset += strtab_len;

    // .shstrtab (Index 7)
    shdrs[7].sh_name = 45;
    shdrs[7].sh_type = SHT_STRTAB;
    shdrs[7].sh_offset = offset;
    shdrs[7].sh_size = shstrtab_len;
    shdrs[7].sh_addralign = 1;
    offset += shstrtab_len;

    // Section header table must be 8-byte aligned
    offset = (offset + 7) & ~7;

    // ELF File Header
    Elf64_Ehdr ehdr;
    memset(&ehdr, 0, sizeof(Elf64_Ehdr));
    ehdr.e_ident[0] = 0x7F;
    ehdr.e_ident[1] = 'E';
    ehdr.e_ident[2] = 'L';
    ehdr.e_ident[3] = 'F';
    ehdr.e_ident[4] = 2; // ELFCLASS64
    ehdr.e_ident[5] = 1; // ELFDATA2LSB
    ehdr.e_ident[6] = 1; // EV_CURRENT
    ehdr.e_ident[7] = 0; // ELFOSABI_SYSV
    ehdr.e_type = ET_REL;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_shoff = offset;
    ehdr.e_shentsize = sizeof(Elf64_Shdr);
    ehdr.e_shnum = 8;
    ehdr.e_shstrndx = 7;

    // WRITING DATA TO DISK WITH PROPER PADDING
    uint64_t cur = 0;
    fwrite(&ehdr, 1, sizeof(Elf64_Ehdr), f);
    cur += sizeof(Elf64_Ehdr);

    if (prog->text_len > 0) {
        fwrite(prog->text_bytes, 1, prog->text_len, f);
        cur += prog->text_len;
    }
    if (prog->data_len > 0) {
        fwrite(prog->data_bytes, 1, prog->data_len, f);
        cur += prog->data_len;
    }
    if (prog->rela_count > 0) {
        cur = align_file_offset(f, cur, 8);
        fwrite(prog->relas, 1, shdrs[4].sh_size, f);
        cur += shdrs[4].sh_size;
    }

    cur = align_file_offset(f, cur, 8);
    fwrite(syms, 1, shdrs[5].sh_size, f);
    cur += shdrs[5].sh_size;

    fwrite(strtab_data, 1, strtab_len, f);
    cur += strtab_len;

    fwrite(shstrtab_data, 1, shstrtab_len, f);
    cur += shstrtab_len;

    align_file_offset(f, cur, 8);
    fwrite(shdrs, 1, sizeof(shdrs), f);

    fclose(f);
}