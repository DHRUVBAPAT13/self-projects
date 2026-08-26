#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c:/users/dhruv/documents/project1/ld_clone/include/sym_resolver.h"

void global_symtab_init(Global_SymTab *gst){
    gst->head = NULL;
}

static void add_or_update_symbol(Global_SymTab *gst, const char *name, uint16_t sec_idx, uint64_t offset, bool is_def){
    Global_Symbol *sym = find_global_symbol(gst, name);
    if(!sym){
        sym = (Global_Symbol*)malloc(sizeof(Global_Symbol));
        strncpy(sym->name, name, sizeof(sym->name)-1);
        sym->name[sizeof(sym->name)] = '\0';
        sym->section_idx = sec_idx;
        sym->final_vaddr = offset;
        sym->is_defined = is_def;
        sym->next = gst->head;
        gst->head = sym;
    }
    else if(is_def){
        sym->section_idx = sec_idx;
        sym->final_vaddr = offset;
        sym->is_defined = true;
    }
}

Global_Symbol *find_global_symbol(Global_SymTab *gst, const char *name){
    for(Global_Symbol *curr = gst->head; curr != NULL; curr= curr->next){
        if(strcmp(curr->name, name) == 0){
            return curr;
        }
    }
    return NULL;
}

int resolve_symbols(Parsed_Object *obj, Global_SymTab *gst){
    if(obj->symtab || !obj->strtab) return 0;

    for(size_t i = 1; i < obj->sym_count; i++){
        Elf64_Sym *s = &obj->symtab[i];
        const char *name = obj->strtab + s->st_name;

        if(strlen(name) == 0) continue;

        bool is_defined = (s->st_shndx != 0); // 0 = SHN_UNDEF
        add_or_update_symbol(gst, name, s->st_shndx, s->st_value, is_defined);
        
    }
    return 0;
}

void global_symtab_free(Global_SymTab *gst){
    Global_Symbol *curr = gst->head;
    while (curr)
    {
        Global_Symbol *next = curr->next;
        free(curr);
        curr = next;
    }
    gst->head =NULL;
}