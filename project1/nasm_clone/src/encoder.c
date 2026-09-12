#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "/workspaces/self-projects/project1/nasm_clone/include/encoder.h"

void encode_program_init(Encoded_Program *prog){
    prog->text_capacity = 4096;
    prog->text_bytes = (uint8_t*)malloc(prog->text_capacity);
    prog->text_len = 0;

    prog->data_capacity = 4096;
    prog->data_bytes = (uint8_t*)malloc(prog->data_capacity);
    prog->data_len = 0;

    prog->bss_len = 0;

    prog->rela_capacity = 64;
    prog->relas = (Elf64_Rela*)malloc(sizeof(Elf64_Rela) * prog->rela_capacity);
    prog->rela_count = 0;
}

static void emit_text_byte(Encoded_Program *prog, uint8_t byte){
    if(prog->text_len >= prog->text_capacity){
        prog->text_capacity *= 2;
        prog->text_bytes = (uint8_t*)realloc(prog->text_bytes, prog->text_capacity);
    }
    prog->text_bytes[prog->text_len++] = byte;
}

static void emit_text_bytes(Encoded_Program *prog, const uint8_t *data, size_t len){
    for(size_t i = 0; i < len; i++)
        emit_text_byte(prog, data[i]);
}

static void emit_data_bytes(Encoded_Program *prog, const uint8_t *data, size_t len){
    if(prog->data_len + len > prog->data_capacity){
        prog->data_capacity = (prog->data_len + len)*2;
        prog->data_bytes = (uint8_t*)realloc(prog->data_bytes, prog->data_capacity);
    }
    memcpy(&prog->data_bytes[prog->data_len], data, len);
    prog->data_len += len;
}

static void add_relocation(Encoded_Program *prog, uint64_t offset, uint32_t sym_idx, uint32_t type, int64_t addend){
    if(prog->rela_count >= prog->rela_capacity){
        prog->rela_capacity *= 2;
        prog->relas = (Elf64_Rela*)realloc(prog->relas, sizeof(Elf64_Rela)*prog->rela_capacity);
    }
    Elf64_Rela *rela = &prog->relas[prog->rela_count++];
    rela->r_offset = offset;
    rela->r_info = ELF64_R_INFO(sym_idx, type);
    rela->r_addend = addend;
}

void encode_ast(Instruction_Node *ast, Symbol_Table *st, Encoded_Program *out){
    for(Instruction_Node *curr = ast ; curr != NULL ; curr = curr->next){

        if(strcmp(curr->mnemonic, "db") == 0){
            emit_data_bytes(out, curr->raw_data, curr->raw_data_len);
            continue;
        }

        if(strncmp(curr->mnemonic, "res", 3) == 0){
            out->bss_len += curr->raw_data_len;
        }

        // mov instrction variations
        if(strcmp(curr->mnemonic, "mov") == 0){
            Operand *dest = &curr->ops[0];
            Operand *src = &curr->ops[1];

            // mov reg64, imm64 (or label address) -> REX.W (0x48) | 0xB8 + reg | 8-byte imm
            if(dest->type == OP_REG && dest->size == 8 && (src->type == OP_IMM || src->type == OP_LABEL_REF)){
                emit_text_byte(out, 0x48);
                emit_text_byte(out, 0xb8 + (dest->reg_num & 0x07));

                if(src->type == OP_LABEL_REF){
                    add_relocation(out, out->text_len, 2, R_X86_64_64, 0);  // 2: typically .data symbol index
                    uint64_t placeholder = 0;
                    emit_text_bytes(out, (uint8_t*)&placeholder, sizeof(uint64_t));
                }
                else{
                    emit_text_bytes(out, (uint8_t*)&src->imm_val, sizeof(uint64_t));
                }
                continue;
            }

            // mov reg32, imm32 -> 0xB8 + reg | 4-byte imm
            if(dest->type == OP_REG && (dest->size == 4 || dest->size == 8) && src->type == OP_IMM){
                emit_text_byte(out, 0xb8 + (dest->reg_num & 0x07));
                uint32_t val32 = (uint32_t)src->imm_val;
                emit_text_bytes(out, (uint8_t*)&val32, sizeof(uint32_t));
                continue;
            }
        }

        if(strcmp(curr->mnemonic, "xor") == 0){
            Operand *dest = &curr->ops[0];
            Operand *src = &curr->ops[1];
            if(dest->type == OP_REG && src->type == OP_REG){
                emit_text_byte(out, 0x31);
                uint8_t modrm = 0xc0 | ((src->reg_num & 0x07) << 3) | (dest->reg_num & 0x07);
                emit_text_byte(out, modrm);
                continue;
            }
        }
        
        // syscall -> 0x0f 0x05
        if(strcmp(curr->mnemonic, "syscall") == 0){
            emit_text_byte(out, 0x0f);
            emit_text_byte(out, 0x05);
            continue;
        }

        // ret -> 0xc3
        if(strcmp(curr->mnemonic, "ret") == 0){
            emit_text_byte(out, 0xc3);
            continue;
        }
    }
}

void encode_program_free(Encoded_Program *prog){
    if(prog->text_bytes) free(prog->text_bytes);
    if(prog->data_bytes) free(prog->data_bytes);
    if(prog->relas) free(prog->relas);
}
