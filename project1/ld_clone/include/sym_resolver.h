#ifndef SYM_RESOLVER_H
#define SYM_RESOLVER_H

#include <stdint.h>
#include <stdbool.h>
#include "elf_parser.h"

typedef struct Global_Symbol {
    char name[64];
    uint64_t final_vaddr;
    uint16_t section_idx; // 1 = text, 2 = data
    bool is_defined;
    struct Global_Symbol *next;

}Global_Symbol;

typedef struct{
    Global_Symbol *head;
}Global_SymTab;

void global_symtab_init(Global_SymTab *gst);
int resolve_symbols(Parsed_Object *obj, Global_SymTab *gst);
Global_Symbol *find_global_symbol(Global_SymTab *gst, const char *name);
void global_symtab_free(Global_SymTab *gst);

#endif /* SYM_RESOLVER_H */