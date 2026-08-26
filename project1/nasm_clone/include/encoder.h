#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include <stddef.h>
#include "ast.h"
#include "symtab.h"
#include "elf64.h"

// Define a simple internal relocation struct or include elf64.h

typedef struct{
    uint8_t *text_bytes;
    size_t text_len;
    size_t text_capacity;

    uint8_t *data_bytes;
    size_t data_len;
    size_t data_capacity;

    size_t bss_len; 

    Elf64_Rela *relas;
    size_t rela_count;
    size_t rela_capacity;

}Encoded_Program;

void encode_program_init(Encoded_Program *prog);
void encode_ast(Instruction_Node *ast, Symbol_Table *st, Encoded_Program *out);
void encode_program_free(Encoded_Program *prog);


#endif /* ENCODER_H */