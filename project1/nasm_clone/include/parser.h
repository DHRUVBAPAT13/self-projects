#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include "symtab.h"

Instruction_Node *parse_source(Lexer *l, Symbol_Table *st);
void free_ast(Instruction_Node *head);

#endif /* PARSER_H */