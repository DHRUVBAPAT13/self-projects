#ifndef ELF64_H
#define ELF64_H

#include <stdint.h>
#include <stddef.h>

// ELF indetification INDEXES
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_MOSABI 7
#define EI_NIDENT 16

// MAGIC NUMBERS
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

// ARCHITECHTURE & DATA ENCODING
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EV_CURRENT 1
#define ELFOSABI_SYSV 0

// FILE TYPES & MACHINE TYPES
#define ET_REL 1
#define ET_EXEC 2
#define EM_X86_64 62

// SECTION HEADER TYPES
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_NOBITS 8

//SECTION HEADER FLAGS
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4

// SYMBOL BINDING & TYPES
#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_NOTYPE 0

#define ELF64_ST_INFO(bind, type)   (((bind) << 4) + ((type) & 0xf))
#define ELF64_R_INFO(sym, type) ((((uint64_t)(sym)) << 32) + (uint32_t)(type))

// x86_64 RELOCATION TYPES
#define R_X86_64_64 1
#define R_X86_64_PC32 2
#define R_X86_64_32S 11

#pragma pack(push, 1)

typedef struct 
{
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;

}Elf64_Ehdr;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;

} Elf64_Shdr;

typedef struct 
{
    uint32_t st_name;
    unsigned char st_info;
    unsigned char st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;

}Elf64_Sym;

typedef struct{
    uint64_t r_offset;
    uint64_t r_info;
    int64_t r_addend;

}Elf64_Rela;

#pragma pack(pop)

#endif /* ELF64_H */