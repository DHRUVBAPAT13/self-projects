#include <string.h>
#include "/workspaces/self-projects/project1/ld_clone/include/layout.h"
#include "/workspaces/self-projects/project1/ld_clone/include/linker.h"

void plan_layout(Parsed_Object *obj, Global_SymTab *gst, Memory_Layout *layout){
    
    memset(layout, 0, sizeof(Memory_Layout));

    layout->base_vaddr = DEFAULT_BASE_VADDR;

    // headers : ELF header(64 bytes) + Program header(56 bytes) = 120 bytes
    uint64_t header_size = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);

    // .text section sits directly after headers
    layout->text_offset = header_size;
    layout->text_vaddr = layout->base_vaddr + layout->text_offset;
    layout->text_size = obj->text_size;

    // .data section sits immediately after .text
    layout->data_offset = layout->text_offset + layout->text_size;
    layout->data_vaddr = layout->base_vaddr + layout->data_offset;
    layout->data_size = obj->data_size;

    // .bss sits directly after .data in virtual memory
    layout->bss_vaddr = layout->data_vaddr + layout->data_size;
    layout->bss_size = obj->bss_size;

    // File size (.text + .data) vs Memory size (.text + .data + .bss)
    layout->total_file_size = layout->data_offset + layout->data_size;
    layout->total_mem_size = layout->total_file_size + obj->bss_size;

    // assign final virtual memory addresses to symbols across all sections
    for(Global_Symbol *sym  = gst->head; sym != NULL; sym = sym->next){
        if(sym->section_idx == 1){
            sym->final_vaddr = layout->text_vaddr + sym->final_vaddr;
        }
        else if(sym->section_idx == 2){
            sym->final_vaddr = layout->data_vaddr + sym->final_vaddr;
        }
        else if(sym->section_idx == 3){
            sym->final_vaddr = layout->bss_vaddr + sym->final_vaddr;
        }
    }

    Global_Symbol *start_sym = find_global_symbol(gst, "_start");
    if(start_sym && start_sym->is_defined){
        layout->entry_point = start_sym->final_vaddr;
    }
    else{
        layout->entry_point = layout->text_vaddr;
    }
}