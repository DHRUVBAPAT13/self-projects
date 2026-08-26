#ifndef LAYOUT_H
#define LAYOUT_H

#include <stdint.h>
#include <stddef.h>
#include "elf_parser.h"
#include "sym_resolver.h"

#define DEFAULT_BASE_VADDR 0x400000
#define PAGE_SIZE 0x1000

typedef struct{
    uint64_t base_vaddr;

    uint64_t text_vaddr;
    uint64_t text_offset;
    size_t text_size;

    uint64_t data_vaddr;
    uint64_t data_offset;
    size_t data_size;

    uint64_t bss_vaddr;
    size_t bss_size;

    uint64_t entry_point;
    size_t total_file_size;
    size_t total_mem_size;
}Memory_Layout;

void plan_layout(Parsed_Object *obj, Global_SymTab *gst, Memory_Layout *layout);

#endif /* LAYOUT_H */