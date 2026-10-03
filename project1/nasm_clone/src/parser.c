#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "/workspaces/self-projects/project1/nasm_clone/include/parser.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

typedef struct{
    const char *name;
    uint8_t reg_num;
    uint8_t size;

}Reg_Entry;

static const Reg_Entry REG_TABLE[] = {
    {"rax", 0, 8}, {"rcx", 1, 8}, {"rdx", 2, 8}, {"rbx", 3, 8},
    {"rsp", 4, 8}, {"rbp", 5, 8}, {"rsi", 6, 8}, {"rdi", 7, 8},
    {"eax", 0, 4}, {"ecx", 1, 4}, {"edx", 2, 4}, {"ebx", 3, 4},
    {"esp", 4, 4}, {"ebp", 5, 4}, {"esi", 6, 4}, {"edi", 7, 4},
    {"al",  0, 1}, {"cl",  1, 1}, {"dl",  2, 1}, {"bl",  3, 1},
    {NULL, 0, 0}
};

static bool parse_reg(const char *name, Operand *op){
    for(int i = 0; REG_TABLE[i].name != NULL; i++){
        if(strcasecmp(name, REG_TABLE[i].name) == 0){
            
            op->type = OP_REG;
            op->reg_num = REG_TABLE[i].reg_num;
            op->size = REG_TABLE[i].size;
            return true;
        }
    }
    return false;
}

static Operand parse_operand(Lexer *l){
    Token tok = lexer_next(l);
    Operand op;
    memset(&op, 0, sizeof(Operand));

    if(tok.type == TOK_NUMBER){
        op.type = OP_IMM;
        op.imm_val = tok.int_value;
        return op;
    }

    if(tok.type == TOK_IDENTIFIER){
        if(parse_reg(tok.text, &op)){
            return op;
        }

        op.type = OP_LABEL_REF;
        strncpy(op.label, tok.text, sizeof(op.label)-1);
        return op;
    }
    return op;
}

Instruction_Node *parse_source(Lexer *l, Symbol_Table *st){
    Instruction_Node *head = NULL;
    Instruction_Node *tail = NULL;
    int current_sec_idx = 1; // 1: .text, 2: .data, 3: .bss
    uint64_t current_sec_offset[4] = {0, 0, 0, 0}; // Track offset per section

    while (1)
    {
        Token tok = lexer_next(l);
        if(tok.type == TOK_EOF) break;
        if(tok.type == TOK_NEWLINE) continue;

        // Check for Label Definition:
        // Case A: `_start:` or `msg:` (followed by colon)
        // Case B: `msg db ...` (identifier directly followed by db/dw/dd/dq/resb)
        bool has_label = false;
        char label_name[64] = {0};

        if(tok.type == TOK_IDENTIFIER) {
            Token peek = lexer_peek(l);
            if(peek.type == TOK_COLON) {
                strncpy(label_name, tok.text, sizeof(label_name) - 1);
                lexer_next(l); // consume ':'
                has_label = true;
            } else if(strcmp(peek.text, "db") == 0 || strcmp(peek.text, "dw") == 0 ||
                      strcmp(peek.text, "dd") == 0 || strcmp(peek.text, "dq") == 0 ||
                      strncmp(peek.text, "res", 3) == 0) {
                strncpy(label_name, tok.text, sizeof(label_name) - 1);
                has_label = true;
            }
        }

        if(has_label) {
            symtab_add(st, label_name, current_sec_offset[current_sec_idx], current_sec_idx, false, true);

            // Peek next: if next token is newline or EOF, just continue
            Token next_tok = lexer_peek(l);
            if(next_tok.type == TOK_NEWLINE || next_tok.type == TOK_EOF) {
                continue;
            }
            // Otherwise, get the next token on the same line (e.g. "db")
            tok = lexer_next(l);
        }

        // Handle directives (FIXED: == instead of =)
        if(tok.type == TOK_DIRECTIVE || tok.type == TOK_IDENTIFIER){
            if(strcmp(tok.text, "global") == 0){
                Token sym_tok = lexer_next(l);
                symtab_mark_global(st, sym_tok.text);
                continue;
            }

            if(strcmp(tok.text, "section") == 0){
                Token sec_tok = lexer_next(l);
                if(strcmp(sec_tok.text, ".text") == 0) current_sec_idx = 1;
                else if(strcmp(sec_tok.text, ".data") == 0) current_sec_idx = 2;
                else if(strcmp(sec_tok.text, ".bss") == 0) current_sec_idx = 3;
                continue;
            }

            if(strcmp(tok.text, "db") == 0){
                Instruction_Node *node = (Instruction_Node*)calloc(1, sizeof(Instruction_Node));
                strcpy(node->mnemonic, "db");

                while (1)
                {
                    Token val_tok = lexer_next(l);
                    if(val_tok.type == TOK_STRING){
                        size_t len = strlen(val_tok.text);
                        if(node->raw_data_len + len <= sizeof(node->raw_data)){
                            memcpy(&node->raw_data[node->raw_data_len], val_tok.text, len);
                            node->raw_data_len += len;
                        }
                    }
                    else if(val_tok.type == TOK_NUMBER){
                        if(node->raw_data_len < sizeof(node->raw_data)){
                            node->raw_data[node->raw_data_len++] = (uint8_t)val_tok.int_value;
                        }
                    }
                    if(lexer_peek(l).type == TOK_COMMA){
                        lexer_next(l);
                    }
                    else{
                        break;
                    }
                }

                current_sec_offset[current_sec_idx] += node->raw_data_len;

                if(!head) head = tail = node;
                else {
                    tail->next = node;
                    tail = node;
                }
                continue;
            }

            if (strncmp(tok.text, "res", 3) == 0) {
                int multiplier = 1;
                if (strcmp(tok.text, "resw") == 0) multiplier = 2;
                else if (strcmp(tok.text, "resd") == 0) multiplier = 4;
                else if (strcmp(tok.text, "resq") == 0) multiplier = 8;

                Token count_tok = lexer_next(l);
                size_t total_bytes = (size_t)count_tok.int_value * multiplier;

                Instruction_Node *node = (Instruction_Node*)calloc(1, sizeof(Instruction_Node));
                strcpy(node->mnemonic, tok.text);
                node->raw_data_len = total_bytes;

                current_sec_offset[current_sec_idx] += total_bytes;

                if (!head) head = tail = node;
                else { tail->next = node; tail = node; }
                continue;
            }
        }

        // Standard instructions (mov, syscall, etc.)
        Instruction_Node *node = (Instruction_Node*)calloc(1, sizeof(Instruction_Node));
        strncpy(node->mnemonic, tok.text, sizeof(node->mnemonic)-1);

        if(lexer_peek(l).type != TOK_NEWLINE && lexer_peek(l).type != TOK_EOF){
            node->ops[0] = parse_operand(l);
            node->op_count = 1;

            if(lexer_peek(l).type == TOK_COMMA){
                lexer_next(l);  // consumes ','
                node->ops[1] = parse_operand(l);
                node->op_count = 2;
            }
        }

        if(!head) head = tail = node;
        else { tail->next = node; tail = node; }
    }
    return head;
}

void free_ast(Instruction_Node *head){
    while (head)
    {
        Instruction_Node *next = head->next;
        free(head);
        head = next;
    }
}