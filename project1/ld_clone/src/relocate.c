#include <stdio.h>
#include <string.h>
#include "c:/users/dhruv/documents/project1/ld_clone/include/relocate.h"

int apply_relocations(Parsed_Object *obj, Global_SymTab *gst, const Memory_Layout *layout){
    if(!obj->relas || obj->rela_count == 0) return 0;

    for (size_t i = 0; i < obj->rela_count; i++){

        Elf64_Rela *rela = &obj->relas[i];
        uint32_t sym_idx = (uint32_t)(rela->r_info >> 32);
        uint32_t type = (uint32_t)(rela->r_info & 0xffffffff);

        uint64_t target_vaddr = 0;

        if(sym_idx < obj->sym_count){
            Elf64_Sym *s = &obj->symtab[sym_idx];
            const char *sym_name = obj->strtab + s->st_name;
            Global_Symbol *gsym = find_global_symbol(gst, sym_name);

            if(gsym && gsym->is_defined){
                target_vaddr = gsym->final_vaddr;
            }
            else{
                if(s->st_shndx == 3){
                    target_vaddr = layout->bss_vaddr; // .bss section
                }
                else if(s->st_shndx == 2){
                    target_vaddr = layout->data_vaddr; // .data section
                }
                else if(s->st_shndx == 1){
                    target_vaddr = layout->text_vaddr; // .text section
                }
            }
        }

        uint8_t *patch_loc = obj->text_data + rela->r_offset;

        // R_X86_64_64 : 64-bit absolute address (S + A)
        if(type = R_X86_64_64){
            uint64_t value = target_vaddr + rela->r_addend;
            memcpy(patch_loc, &value, sizeof(uint64_t));
        }

        // R_X86_64_PC32 : 32-bit PC-relative address (S + A - P)           here PC is Program Counter
        else if(type == R_X86_64_PC32){
            uint64_t p_loc = layout->text_vaddr + rela->r_offset;
            int32_t value = (int32_t)(target_vaddr + rela->r_addend - p_loc);
            memcpy(patch_loc, &value, sizeof(int32_t));
        }

        // R_X86_64_32S : 32-bit signed absolute address
        else if(type == R_X86_64_32S){
            int32_t value = (int32_t)(target_vaddr + rela->r_addend);
            memcpy(patch_loc, &value, sizeof(int32_t));
        }
    }
    
    return 0;
}