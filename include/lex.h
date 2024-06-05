#ifndef _AX_LEX_H_
#define _AX_LEX_H_

#include "types.h"

#include <stdio.h>

typedef enum TokenKind {
	// Keywords
	TK_AS,
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
	TK_LAST_KEYWORD = TK_VOID,

	// Operators
	TK_ARROW,
	TK_BAND,
	TK_BAND_EQ,
	TK_BNOT,
	TK_BOR,
	TK_BOR_EQ,
	TK_BRACE_L,
	TK_BRACE_R,
	TK_BRACKET_L,
	TK_BRACKET_R,
	TK_BXOR,
	TK_BXOR_EQ,
	TK_COLON,
	TK_COMMA,
	TK_DOT,
	TK_EQUAL,
	TK_GREATER,
	TK_GREATER_EQ,
	TK_LAND,
	TK_LEQUAL_EQ,
	TK_LESS,
	TK_LESS_EQ,
	TK_LNOT,
	TK_LNOT_EQ,
	TK_LOR,
	TK_MINUS,
	TK_MINUS_EQ,
	TK_MOD,
	TK_MOD_EQ,
	TK_PAREN_L,
	TK_PAREN_R,
	TK_PLUS,
	TK_PLUS_EQ,
	TK_SEMICOLON,
	TK_SHIFTL,
	TK_SHIFTL_EQ,
	TK_SHIFTR,
	TK_SHIFTR_EQ,
	TK_SLASH,
	TK_SLASH_EQ,
	TK_STAR,
	TK_STAR_EQ,
	TK_LAST_OPERATOR = TK_STAR_EQ,

	// Data
	TK_FLOAT,
	TK_IDENTIFIER,
	TK_INTEGER,
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

TokenKind lex_scan(LexState *lex, Token *tok);
const char *lex_tok2str(TokenKind tok);

#endif
