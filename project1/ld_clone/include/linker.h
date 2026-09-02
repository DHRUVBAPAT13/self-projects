#ifndef LINKER_H
#define LINKER_H

#include "elf_parser.h"
#include "sym_resolver.h"
#include "layout.h"

#define PT_NULL 0
#define PT_LOAD 1
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

#pragma pack(push, 1)

typedef struct{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;

}Elf64_Phdr;

#pragma pack(pop)

int write_elf64_execultable(const char *outfile, Parsed_Object *obj, const Memory_Layout *layout);

#endif /* LINKER_H */