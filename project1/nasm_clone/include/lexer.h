#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stddef.h>

typedef enum{
    TOK_EOF,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_COMMA,
    TOK_COLON,
    TOK_NEWLINE,
    TOK_DIRECTIVE

}Token_Type;

typedef struct{
    Token_Type type;
    char text[128];
    int64_t int_value;
    int line;

}Token;

typedef struct {
    const char *src;
    size_t pos;
    int line;

}Lexer;

void lexer_init(Lexer *l, const char *source);

Token lexer_next(Lexer *l);
Token lexer_peek(Lexer *l);

#endif /* LEXER_H */