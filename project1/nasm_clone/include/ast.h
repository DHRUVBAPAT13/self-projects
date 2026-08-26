#ifndef AST_H
#define AST_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum{
    OP_NONE, 
    OP_REG,     // rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp
    OP_IMM,     // 1, 60, 0x10, etc.
    OP_LABRL_REF        // Symbol label reference (e.g. msg, _start)

} Operand_Type;

typedef struct{
    Operand_Type type;
    uint8_t reg_num;     // Numeric identifier: 0=RAX, 1=RCX, 2=RDX, 3=RBX, 6=RSI, 7=RDI
    uint8_t size;       // Operand width: 1=byte, 2=word, 4=double word, 8=quad word
    uint64_t imm_val;
    char label[64];

} Operand;

typedef struct Instruction_Node{
    char mnemonic[16];      // "mov", "syscall", "xor", "db", "section", "global", etc.
    Operand ops[2];
    int op_count;

    // Storage for direct raw data emmited via directives (e.g. db) 
    uint8_t raw_data[256];
    size_t raw_data_len;

    struct Instruction_Node *next;

}Instruction_Node;

#endif /* AST_H */