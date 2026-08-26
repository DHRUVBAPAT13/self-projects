#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "c:/users/dhruv/documents/project1/nasm_clone/include/elf64.h"

typedef struct{
    uint8_t *raw_data;
    size_t file_size;

    Elf64_Ehdr *ehdr;
    Elf64_Shdr *sections;
    uint16_t shnum;

    const char *shstrtab;
    const char *strtab;

    // direct section data pointers
    uint8_t *text_data;
    size_t text_size;
    uint64_t text_offset_in_file;

    uint8_t *data_data;
    size_t data_size;
    uint64_t data_offset_in_file;

    size_t bss_size;

    Elf64_Sym *symtab;
    size_t sym_count;

    Elf64_Rela *relas;
    size_t rela_count;

}Parsed_Object;

int parse_object_file(const char *filename, Parsed_Object *obj);
void free_parsed_object(Parsed_Object *obj);

#endif /* ELF_PARSER_H */