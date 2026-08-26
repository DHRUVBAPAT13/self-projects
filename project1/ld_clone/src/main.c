#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c:/users/dhruv/documents/project1/ld_clone/include/elf_parser.h"
#include "c:/users/dhruv/documents/project1/ld_clone/include/sym_resolver.h"
#include "c:/users/dhruv/documents/project1/ld_clone/include/layout.h"
#include "c:/users/dhruv/documents/project1/ld_clone/include/relocate.h"
#include "c:/users/dhruv/documents/project1/ld_clone/include/linker.h"

int main(int argc, char **argv){

    const char *input_file = NULL;
    const char *output_file = "a.out";

    for(int i = 1; i < argc; i++){
        if((strcmp(argv[i], "-o") == 0) && (i+1 < argc)){
            output_file = argv[++i];
        }
        else{
            input_file = argv[i];
        }
    }

    if(!input_file){
        fprintf(stderr, "Usage : %s -o <output.exe> <input.o>\n",argv[0]);
        return 1;
    }

    // Step 1: parse the input relocatable ELF (.o)
    Parsed_Object obj;
    if(parse_object_file(input_file, &obj) != 0){
        return 1;
    }

    // step 2: build global symbol table and resolve addresses
    Global_SymTab gst;
    global_symtab_init(&gst);
    resolve_symbols(&obj, &gst);

    // step 3: plan virtual memory maaping (.text, .data, .bss)
    Memory_Layout layout;
    plan_layout(&obj, &gst, &layout);

    // step 4: apply relocations to machine code in memory
    apply_relocations(&obj, &gst, &layout);

    // step 5: write the final executable ELF
    if(write_elf64_execultable(output_file, &obj, &layout) == 0){

        printf("Successfully linked %s -> %s\n",input_file, output_file);
    }

    // cleanup resources
    global_symtab_free(&gst);
    free_parsed_object(&obj);

    return 0;
    
}