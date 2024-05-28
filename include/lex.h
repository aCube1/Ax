#ifndef _AX_LEX_H_
#define _AX_LEX_H_

#include "types.h"

#include <stdio.h>

typedef enum TokenKind {
	// Keywords
	TK_BOOL,
	TK_CONST,
	TK_F32,
	TK_F64,
	TK_FALSE,
	TK_FOR,
	TK_FUNC,
	TK_I16,
	TK_I32,
	TK_I64,
	TK_I8,
	TK_IF,
	TK_IMPORT,
	TK_MUT,
	TK_PUB,
	TK_TRUE,
	TK_U16,
	TK_U32,
	TK_U64,
	TK_U8,
	TK_VOID,
	TK_KEYWORDS_COUNT,

	// Symbols
	TK_ARROW,
	TK_ASTERISK,
	TK_COMMA,
	TK_COLON,
	TK_COLONEQ,
	TK_EQUAL,
	TK_EQUALEQ,
	TK_LBRACE,
	TK_LESS,
	TK_LESSEQ,
	TK_LPAREN,
	TK_PLUS,
	TK_PLUSEQ,
	TK_RBRACE,
	TK_RPAREN,
	TK_SEMICOLON,
	TK_SLASH,
	TK_SYMBOLS_COUNT,

	// Data
	TK_IDENTIFIER,
	TK_NUMBER,
	TK_STRING,

	// Misc.
	TK_NONE,
	TK_EOF,
} TokenKind;

typedef struct Location {
	int lineno;
	int colno;
} Location;

typedef struct Token {
	TokenKind kind;
	Location loc;

	// Data
	union {
		char *ident; // Identifier name
		f64 fval;
		u64 uval;
		i64 ival;

		struct {
			usize len;
			char *ptr;
		} str;
	};
} Token;

typedef struct LexState {
	FILE *file;
	Location loc;

	char stack[2];
	usize buflen;
	usize bufsize;
	char *buf;
} LexState;

void lex_init(LexState *lex, FILE *file);
void lex_close(LexState *lex);

TokenKind lex_scan(LexState *lex, Token *out);

#endif
