#include <stdlib.h>
#include <string.h>
#include "c:/users/dhruv/documents/project1/nasm_clone/include/symtab.h"

void symtab_init(Symbol_Table *st){
    st->head = NULL;
}

void symtab_add(Symbol_Table *st, const char *name, uint64_t offset, uint16_t sec_idx, bool is_global, bool is_defined){

    Symbol *sym = symtab_find(st, name);
    if(!sym){
        sym = (Symbol*)malloc(sizeof(Symbol));
        strncpy(sym->name, name, sizeof(sym->name)-1);
        sym->name[sizeof(sym->name-1)] = '\0';
        sym->offset = offset;
        sym->section_idx = sec_idx;
        sym->is_global = is_global;
        sym->is_defined = is_defined;
        sym->next = st->head;
        st->head = sym;
    }
    else{
        if(is_defined){
            sym->offset = offset;
            sym->section_idx = sec_idx;
            sym->is_defined = true;
        }
        if(is_global){
            sym->is_global = true;
        }
    }
}

Symbol *symtab_find(Symbol_Table *st, const char *name){
    for(Symbol *curr = st->head; curr != NULL; curr = curr->next)
    {
        if(strcmp(curr->name, name) == 0){
            return curr;
        }
    }
    return NULL;
}

void symtab_mark_global(Symbol_Table *st, const char *name){
    Symbol *sym = symtab_find(st, name);
    if(sym){
        sym->is_global = true;
    } 
    else{
        symtab_add(st, name, 0, 0, true, false);
    }
}

void symtab_free(Symbol_Table *st){
    Symbol *curr = st->head;
    while(curr){
        Symbol *next = curr->next;
        free(curr);
        curr = next;
    }
    st->head = NULL;
}