#include "c:/users/dhruv/documents/project1/nasm_clone/include/lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void lexer_init(Lexer *l, const char *source){
    l->src = source;
    l->pos = 0;
    l->line = 1;
}

static char peek_char(Lexer *l){
    return l->src[l->pos];
}

static char next_char(Lexer *l){
    char c = l->src[l->pos];
    if(c != '\0'){
        l->pos++;
        if(c == '\n')
            l->line++;
    }
    return c;
}

static void skip_whitespace_and_comments(Lexer *l){
    while(1){
        char c = peek_char(l);
        if(c == ' ' || c== '\t' || c == '\r'){
            next_char(l);
        }
        else if(c == ';'){
            while(peek_char(l) != '\n' && peek_char(l) != '\0'){
                next_char(l);
            }
        }
        else{
            break;
        }
    }
}

Token lexer_next(Lexer *l){
    Token tok;

	memset(&tok, 0, sizeof(Token));

	skip_whitespace_and_comments(l);
	tok.line = l->line;

	char c = peek_char(l);
	if (c == '\0') {
        tok.type = TOK_EOF;
        return tok;
		
    }

	if (c == '\n') {
        next_char(l);
        tok.type = TOK_NEWLINE;
        strcpy(tok.text, "\\n");
        return tok;

    }

	if (c == ',') {
        next_char(l);
        tok.type = TOK_COMMA;
        strcpy(tok.text, ",");
        return tok;

    }

	if (c == ':') {
        next_char(l);
        tok.type = TOK_COLON;
        strcpy(tok.text, ":");
        return tok;

    }

	// Parse String Literal
    if (c == '"' || c == '\'') {
        char quote = next_char(l);
        int idx = 0;
        while (peek_char(l) != quote && peek_char(l) != '\0') {
            char ch = next_char(l);
            if (ch == '\\') {
                char esc = next_char(l);
                if (esc == 'n') ch = '\n';
                else if (esc == 't') ch = '\t';
                else if (esc == 'r') ch = '\r';
                else if (esc == '0') ch = '\0';
                else ch = esc;
            }
            if (idx < (int)sizeof(tok.text) - 1) {
                tok.text[idx++] = ch;
            }
        }
        if (peek_char(l) == quote) next_char(l);
        tok.text[idx] = '\0';
        tok.type = TOK_STRING;
        return tok;

    }

	// Parse Hex or Decimal Number
    if (isdigit((unsigned char)c) || (c == '-' && isdigit((unsigned char)l->src[l->pos + 1]))) {
        int idx = 0;
        if (c == '-') tok.text[idx++] = next_char(l);
        while (isxdigit((unsigned char)peek_char(l)) || peek_char(l) == 'x' || peek_char(l) == 'X') {
            if (idx < (int)sizeof(tok.text) - 1) {
                tok.text[idx++] = next_char(l);
            }
        }
        tok.text[idx] = '\0';
        tok.type = TOK_NUMBER;
        tok.int_value = strtoll(tok.text, NULL, 0);
        return tok;

    }

	// Parse Identifier / Directive / Keyword
    if (isalpha((unsigned char)c) || c == '_' || c == '.') {
        int idx = 0;
        while (isalnum((unsigned char)peek_char(l)) || peek_char(l) == '_' || peek_char(l) == '.') {
            if (idx < (int)sizeof(tok.text) - 1) {
                tok.text[idx++] = next_char(l);
            }
        }
        tok.text[idx] = '\0';

        if (strcmp(tok.text, "global") == 0 || strcmp(tok.text, "section") == 0 || strcmp(tok.text, "db") == 0 || strcmp(tok.text, "extern") == 0 || strcmp(tok.text, "resb") == 0 || 
            strcmp(tok.text, "resw") == 0 || strcmp(tok.text, "resd") == 0 || strcmp(tok.text, "resq") == 0) {
            tok.type = TOK_DIRECTIVE;
        } 
		else {
            tok.type = TOK_IDENTIFIER;
        }
        return tok;

    }

	// Fallback for unexpected characters
    tok.text[0] = next_char(l);
    tok.text[1] = '\0';
    tok.type = TOK_IDENTIFIER;
    return tok;

}

Token lexer_peek(Lexer *l) {
    size_t saved_pos = l->pos;
    int saved_line = l->line;
    Token tok = lexer_next(l);
    l->pos = saved_pos;
    l->line = saved_line;
    return tok;

}