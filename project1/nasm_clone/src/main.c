#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "/workspaces/self-projects/project1/nasm_clone/include/lexer.h"
#include "/workspaces/self-projects/project1/nasm_clone/include/parser.h"
#include "/workspaces/self-projects/project1/nasm_clone/include/encoder.h"
#include "/workspaces/self-projects/project1/nasm_clone/include/symtab.h"

extern void write_elf64_object(const char *filename, Encoded_Program *prog, Symbol_Table *st);

static char *read_entire_file(const char *path){
    FILE *f = fopen(path, "rb");
    if(!f) return NULL;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buff = (char*)malloc(sz + 1);
    fread(buff, 1, sz, f);
    buff[sz] = '\0';
    fclose(f);
    return buff;
}

int main(int argc, char **argv){
    const char *input_file = NULL;
    const char *output_file = "out.o";

    for(int i = 1; i<argc; i++){
        if(strcmp(argv[i], "-f") == 0 && i+1 < argc){
            i++;
        }
        else if(strcmp(argv[i], "-o") == 0 && i+1 < argc){
            output_file = argv[++i];
        }
        else{
            input_file = argv[i];
        }
    }

    if(!input_file){
        fprintf(stderr, "Usage : %s -f elf64 <input.asm> -o <output.o>\n",argv[0]);
        return 1;
    }

    char *source = read_entire_file(input_file);

    if(!source){
        fprintf(stderr, "Error : Could not read file '%s'\n",input_file);
        return 1;
    }

    Lexer lexer;
    lexer_init(&lexer, source);

    Symbol_Table symtab;
    symtab_init(&symtab);

    Instruction_Node *ast = parse_source(&lexer, &symtab);

    Encoded_Program prog;
    encode_program_init(&prog);
    encode_ast(ast, &symtab, &prog);

    write_elf64_object(output_file, &prog, &symtab);
    printf("Successfully assembled : %s -> %s\n",input_file, output_file);

    free_ast(ast);
    encode_program_free(&prog);
    symtab_free(&symtab);
    free(source);
    return 0;

}