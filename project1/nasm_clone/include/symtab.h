#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct Symbol{
    char name[64];
    uint64_t offset;
    uint16_t section_idx;
    bool is_global;
    bool is_defined;
    struct Symbol *next;

}Symbol;

typedef struct{
    Symbol *head;
}Symbol_Table;

void symtab_init(Symbol_Table *st);
void symtab_add(Symbol_Table *st, const char *name, uint64_t offset, uint16_t sec_idx, bool is_global, bool is_defined);
Symbol *symtab_find(Symbol_Table *st, const char *name);
void symtab_mark_global(Symbol_Table *st, const char *name);
void symtab_free(Symbol_Table *st);

#endif /* SYMTAB_H */