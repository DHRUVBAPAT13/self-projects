#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "/workspaces/self-projects/project1/ld_clone/include/elf_parser.h"

int parse_object_file(const char *filename, Parsed_Object *obj){
    memset(obj, 0, sizeof(Parsed_Object));

    FILE *f = fopen(filename, "rb");
    if(!f){
        fprintf(stderr,"ld : error : cannot open input file %s\n", filename);
        return -1;
    }

    fseek(f, 0 ,SEEK_END);
    obj->file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    obj->raw_data = (uint8_t*)malloc(obj->file_size);
    if(fread(obj->raw_data, 1, obj->file_size, f) != obj->file_size){
        fprintf(stderr, "ld : error : failed reading %s\n",filename);
        fclose(f);
        return -1;
    }
    fclose(f);

    obj->ehdr = (Elf64_Ehdr*)obj->raw_data;

    // validate ELF magic bytes
    if(obj->ehdr->e_ident[EI_MAG0] != ELFMAG0 || obj->ehdr->e_ident[EI_MAG1] != ELFMAG1 || 
        obj->ehdr->e_ident[EI_MAG2] != ELFMAG2 || obj->ehdr->e_ident[EI_MAG3] != ELFMAG3){

            fprintf(stderr, "ld : error : '%s' is not a valid ELF file\n", filename);
            return -1;
    }

    if(obj->ehdr->e_type != ET_REL){
        fprintf(stderr, "ld : error : '%s' is not a relocatable file\n", filename);
        return -1;
    }

    obj->sections = (Elf64_Shdr*)(obj->raw_data + obj->ehdr->e_shoff);
    obj->shnum = obj->ehdr->e_shnum;

    // get section header tstring table (.shstrtab)
    if(obj->ehdr->e_shstrndx < obj->shnum){
        obj->shstrtab = (const char*)(obj->raw_data + obj->sections[obj->ehdr->e_shstrndx].sh_offset);
    }

    // inspect and extract individual sections (.text, .data, .bss, .symtab, .strtab, .rela.text)
    for(int i = 0; i < obj->shnum; i++){
        Elf64_Shdr *sh = &obj->sections[i];
        const char *name = obj->shstrtab ? (obj->shstrtab + sh->sh_name) : "";

        if(strcmp(name, ".text") == 0){
            obj->text_data = obj->raw_data + sh->sh_offset;
            obj->text_size = sh->sh_size;
            obj->text_offset_in_file = sh->sh_offset;
        }
        else if(strcmp(name, ".data") == 0){
            obj->data_data = obj->raw_data + sh->sh_offset;
            obj->data_size = sh->sh_size;
            obj->data_offset_in_file = sh->sh_offset;
        }
        else if(strcmp(name, ".bss") == 0){
            obj->bss_size = sh->sh_size;
        }
        else if(sh->sh_type == SHT_SYMTAB){
            obj->symtab = (Elf64_Sym*)(obj->raw_data + sh->sh_offset);
            obj->sym_count = sh->sh_size/sizeof(Elf64_Sym);
        }
        else if((sh->sh_type == SHT_STRTAB) && (i != obj->ehdr->e_shstrndx)){
            obj->strtab = (const char*)(obj->raw_data + sh->sh_offset);
        }
        else if(sh->sh_type == SHT_RELA){
            obj->relas = (Elf64_Rela*)(obj->raw_data + sh->sh_offset);
            obj->rela_count = sh->sh_size/sizeof(Elf64_Rela);
        }
    }
    return 0;
}

void free_parsed_object(Parsed_Object *obj){
    if(obj->raw_data){
        free(obj->raw_data);
        obj->raw_data = NULL;
    }
}