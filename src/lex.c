// Code stolen from/inspired by:
// https://git.sr.ht/~sircmpwn/harec/tree/master/item/src/lex.c

#include "lex.h"

#include "utf8.h"
#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>

static const char *tokens[] = {
	// Keywords (Must be sorted)
	[TK_AS] = "as",
	[TK_BOOL] = "bool",
	[TK_CONST] = "const",
	[TK_F32] = "f32",
	[TK_F64] = "f64",
	[TK_FALSE] = "false",
	[TK_FN] = "fn",
	[TK_FOR] = "for",
	[TK_I16] = "i16",
	[TK_I32] = "i32",
	[TK_I64] = "i64",
	[TK_I8] = "i8",
	[TK_IF] = "if",
	[TK_MUT] = "mut",
	[TK_PACKAGE] = "package",
	[TK_PUB] = "pub",
	[TK_RUNE] = "rune",
	[TK_TRUE] = "true",
	[TK_U16] = "u16",
	[TK_U32] = "u32",
	[TK_U64] = "u64",
	[TK_U8] = "u8",
	[TK_USE] = "use",
	[TK_VOID] = "void",

	// Operators
	[TK_ARROW] = "->",
	[TK_BAND] = "&",
	[TK_BAND_EQ] = "&=",
	[TK_BNOT] = "~",
	[TK_BOR] = "|",
	[TK_BOR_EQ] = "|=",
	[TK_BRACE_L] = "{",
	[TK_BRACE_R] = "}",
	[TK_BRACKET_L] = "[",
	[TK_BRACKET_R] = "]",
	[TK_BXOR] = "^",
	[TK_BXOR_EQ] = "^=",
	[TK_COLON] = ":",
	[TK_COLON2] = "::",
	[TK_COMMA] = ",",
	[TK_DOT] = ".",
	[TK_EQUAL] = "=",
	[TK_GREATER] = ">",
	[TK_GREATER_EQ] = ">=",
	[TK_LAND] = "&&",
	[TK_LEQUAL_EQ] = "==",
	[TK_LESS] = "<",
	[TK_LESS_EQ] = "<=",
	[TK_LNOT] = "!",
	[TK_LNOT_EQ] = "!=",
	[TK_LOR] = "||",
	[TK_MINUS] = "-",
	[TK_MINUS_EQ] = "-=",
	[TK_MOD] = "%",
	[TK_MOD_EQ] = "%=",
	[TK_PAREN_L] = "(",
	[TK_PAREN_R] = ")",
	[TK_PLUS] = "+",
	[TK_PLUS_EQ] = "+=",
	[TK_SEMICOLON] = ";",
	[TK_SHIFTL] = "<<",
	[TK_SHIFTL_EQ] = "<<=",
	[TK_SHIFTR] = ">>",
	[TK_SHIFTR_EQ] = ">>=",
	[TK_SLASH] = "/",
	[TK_SLASH_EQ] = "/=",
	[TK_STAR] = "*",
	[TK_STAR_EQ] = "*=",
};

static_assert(
	sizeof(tokens) / sizeof(const char *) == TK_LAST_OPERATOR + 1,
	"Tokens array doesn't have the same size of Tokens Enum."
);

static _Noreturn void push_error(Location loc, const char *fmt, ...) {
	fprintf(stderr, "%d:%d ", loc.lineno, loc.colno);

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");

	exit(EXIT_FAILURE);
}

static void buffer_insert(LexState *lex, const char *s, usize size) {
	if (lex->buflen + size >= lex->bufsize) {
		lex->bufsize *= 2;
		lex->buf = xrealloc(lex->buf, lex->bufsize);
	}

	memcpy(lex->buf + lex->buflen, s, size);
	lex->buflen += size;
	lex->buf[lex->buflen] = '\0';
}

static void buffer_clear(LexState *lex) {
	lex->buflen = 0;
	lex->buf[0] = '\0';
}

static void stack_push(LexState *lex, u32 c, bool frombuf) {
	assert(lex->stack[1] == UTF8_INVALID);

	lex->stack[1] = lex->stack[0];
	lex->stack[0] = c;

	if (frombuf) { // Consume the character from buffer
		lex->buflen -= 1;
		lex->buf[lex->buflen] = '\0';
	}
}

static void update_line(Location *loc, u32 c) {
	if (c == '\n') { // Update line number and reset column
		loc->lineno += 1;
		loc->colno = 0;
	} else if (c == '\t') {
		loc->colno += 4; // All tabs count as 4 columns
	} else {
		loc->colno += 1;
	}
}

static u32 nextchr(LexState *lex, Location *loc, bool buffer) {
	u32 c;

	if (lex->stack[0] != UTF8_INVALID) {
		c = lex->stack[0];
		lex->stack[0] = lex->stack[1];
		lex->stack[1] = UTF8_INVALID;
	} else {
		c = u8_get(lex->file);
		update_line(&lex->loc, c);

		if (c == UTF8_INVALID && !feof(lex->file)) {
			push_error(*loc, "Invalid UTF-8 sequence found");
		}

		if (c == UTF8_INVALID) {
			log_trace("%c", c);
		}
	}

	if (loc != NULL) {
		*loc = lex->loc;
		for (usize i = 0; i < 2 && lex->stack[i] != UTF8_INVALID; i += 1) {
			update_line(&lex->loc, lex->stack[i]);
		}
	}

	// Check if we need to store the character in the buffer
	if (buffer && c != '\0') {
		buffer_insert(lex, (char *)&c, 1);
	}

	return c;
}

static inline bool is_space(u32 c) {
	return c == ' ' || c == '\t' || c == '\n';
}

static u32 trimspaces(LexState *lex, Location *loc) {
	u32 c = ' ';

	while (c != UTF8_EOF && is_space(c)) {
		c = nextchr(lex, loc, false);
	}

	return c;
}

static TokenKind lex_number(LexState *lex, Token *out) {
	enum Base {
		B_BIN = 0x01,                  // Binary
		B_OCT = 0x02,                  // Octal
		B_HEX = 0x04,                  // Hexadecimal
		B_DEC = B_BIN | B_OCT | B_HEX, // Decimal
		B_MASK = B_DEC,
	};

	enum StateFlag {
		F_SYM = 0x08, // Is Symbol
		F_FLT = 0x10, // Is Float
		F_EXP = 0x20, // Is Exponent
		F_SEP = 0x40, // Is Separator
	};

	static const char numbers[][24] = {
		[B_BIN] = "01",
		[B_OCT] = "01234567",
		[B_HEX] = "0123456789abcdefABCDEF",
		[B_DEC] = "0123456789",
	};

	// NOTE: I think could be a better way to do this, but...
	// I stole this code from the hare-c compiler
	static const char valid_states[0x80][7] = {
		['.'] = { B_DEC, B_HEX, 0 },
		['e'] = { B_DEC, B_DEC | F_FLT, 0 },
		['E'] = { B_DEC, B_DEC | F_FLT, 0 },
		['p'] = { B_HEX, B_HEX | F_FLT, 0 },
		['P'] = { B_HEX, B_HEX | F_FLT, 0 },
		['+'] = { B_DEC | F_EXP | F_SYM, B_DEC | F_FLT | F_EXP | F_SYM, 0 },
		['-'] = { B_DEC | F_EXP | F_SYM, B_DEC | F_FLT | F_EXP | F_SYM, 0 },
		['_'] = { B_BIN, B_OCT, B_HEX, B_DEC, B_DEC | F_FLT, B_HEX | F_FLT, 0 },
	};

	u32 c = nextchr(lex, &out->loc, true);
	assert(c != UTF8_EOF && c <= 0x7f && isdigit(c));

	enum Base state = B_DEC;
	u8 base = 10;

	if (c == '0') {
		c = nextchr(lex, NULL, true);

		if (isdigit(c) || c == '_') {
			push_error(out->loc, "Leading zero in decimal literal");
		} else if (c == 'b') {
			state = B_BIN | F_SYM;
			base = 2;
		} else if (c == 'o') {
			state = B_OCT | F_SYM;
			base = 8;
		} else if (c == 'x') {
			state = B_HEX | F_SYM;
			base = 16;
		}
	}

	if (state != B_DEC) { // Get the next character if base was specified
		c = nextchr(lex, NULL, true);
	}

	usize exp_idx = 0; // Index in buffer where the exponent start
	while (c != UTF8_EOF) {
		// Check if it a valid number in current base
		if (strchr(numbers[state & B_MASK], (i32)c) != NULL) {
			state &= ~(F_SYM | F_SEP);
			c = nextchr(lex, NULL, true);
			continue;
		}

		if ((state & F_SEP) > 0) {
			// The current state is a separator, but didn't found a digit after it
			push_error(out->loc, "Expected digit, found: '%c'", c);
		}

		if (strchr(valid_states[(u8)c], state) == NULL) {
			break;
		}

		switch (c) {
		case '_':
			state |= F_SEP;

			// Consume separator
			lex->buflen -= 1;
			lex->buf[lex->buflen] = '\0';
			break;
		case '-':
		case '+':
			state |= F_SEP;
			// FALLTHROUGH
		case 'e':
		case 'E':
		case 'p':
		case 'P':
			exp_idx = lex->buflen - 1;
			state |= B_DEC | F_EXP;
			// FALLTHROUGH
		case '.':
			state |= F_FLT;
			break;
		default:
			break;
		}

		state |= F_SYM;
		c = nextchr(lex, NULL, true);
	}

	if (c != UTF8_EOF) {
		stack_push(lex, c, true);
	}

	out->kind = TK_CCONST;

	errno = 0;
	if ((state & F_FLT) > 0) {
		out->fval = strtod(lex->buf, NULL);
		out->storage = TYPE_FLOAT;
	} else {
		u64 exp = 0;
		if (exp_idx != 0) {
			exp = strtoumax(lex->buf + exp_idx, NULL, 10);
		}

		out->uval = strtoumax(lex->buf + (base == 10 ? 0 : 2), NULL, base);
		out->storage = TYPE_INT;

		if (out->uval != 0) {
			// Compute exponent if the value is not 0
			for (u64 i = 0; i < exp; i += 1) {
				u64 prev = out->uval;
				out->uval *= 10;

				// Verify for overflow
				if (out->uval / 10 != prev) {
					errno = ERANGE;
					out->uval = INT64_MAX;
				}
			}
		}

		// Try to find the storage size
		if (out->uval > (u64)INT64_MAX) {
			out->storage = TYPE_U64;
		}
	}

	if (errno == ERANGE) {
		push_error(out->loc, "Integer constant overflow");
	}

	buffer_clear(lex);
	return out->kind;
}

static int keyword_cmp(const void *v1, const void *v2) {
	return strcmp(*(const char **)v1, *(const char **)v2);
}

static TokenKind lex_identifier(LexState *lex, Token *out) {
	u32 c = nextchr(lex, &out->loc, true);
	assert(c != UTF8_EOF && c <= 0x7f && (isalpha(c) || c == '_'));

	while (c != UTF8_EOF) {
		if (c > 0x7f || (!isalnum(c) && c != '_')) {
			// We found a invalid identifier symbol
			stack_push(lex, c, true);
			break;
		}

		c = nextchr(lex, NULL, true);
	}

	const char **token = bsearch(
		&lex->buf, tokens, TK_LAST_KEYWORD + 1, sizeof(tokens[0]), keyword_cmp
	);

	if (token != NULL) {
		// Calculate the difference between the found keyword address to the
		// address of the first keyword in the list.
		// Ex:
		// 	token = 0xfff2; tokens = 0xfff0
		// 	token - tokens -> 0xfff2 - 0xfff0 => 2
		out->kind = token - tokens;
	} else {
		// We didn't found a matching keyword, so we treat it as a identifier
		out->kind = TK_IDENTIFIER;
		out->ident = xstrndup(lex->buf, lex->buflen);
	}

	buffer_clear(lex);
	return out->kind;
}

static usize lex_rune(LexState *lex, char *out) {
	u32 c = nextchr(lex, NULL, false);
	assert(c != UTF8_EOF);

	// Parse the escape characters
	if (c == '\\') {
		Location loc = lex->loc;
		char buf[9] = { 0 };
		char *endptr = NULL;

		c = nextchr(lex, NULL, false);

		switch (c) {
		case 'n':
			out[0] = '\n';
			return 1;
		case 'r':
			out[0] = '\r';
			return 1;
		case 't':
			out[0] = '\t';
			return 1;
		case '\\':
			out[0] = '\\';
			return 1;
		case '\'':
			out[0] = '\'';
			return 1;
		case '"':
			out[0] = '\"';
			return 1;
		case '0':
			out[0] = '\0';
			return 1;
		case 'x':
			buf[0] = (char)nextchr(lex, NULL, false);
			buf[1] = (char)nextchr(lex, NULL, false);
			buf[2] = '\0';

			c = strtoul(buf, &endptr, 16);
			if (*endptr != '\0') {
				push_error(loc, "Invalid hex escape sequence");
			}

			out[0] = (char)c;
			return 1;
		case 'u':
			for (u32 i = 0; i < 4; i += 1) {
				buf[i] = (char)nextchr(lex, NULL, false);
			}
			buf[4] = '\0';

			c = strtoul(buf, &endptr, 16);
			if (*endptr != '\0') {
				push_error(loc, "Invalid hex escape sequence");
			}

			return u8_encode(out, c);
		case 'U':
			for (u32 i = 0; i < 8; i += 1) {
				buf[i] = (char)nextchr(lex, NULL, false);
			}
			buf[8] = '\0';

			c = strtoul(out, &endptr, 16);
			if (*endptr != '\0') {
				push_error(loc, "Invalid hex escape sequence");
			}

			return u8_encode(buf, c);
		case UTF8_EOF:
			push_error(lex->loc, "Unexpected end of file");
		default:
			push_error(loc, "Invalid escape sequence '\\%c'", c);
		}
	}

	return u8_encode(out, c);
}

static TokenKind lex_string(LexState *lex, Token *out) {
	u32 c = nextchr(lex, &out->loc, false);
	char buf[UTF8_MAXBYTES + 1] = { 0 };

	switch (c) {
	case '"':
		c = nextchr(lex, NULL, false);
		while (c != '"') {
			if (c == UTF8_EOF) {
				push_error(out->loc, "Unexpected end of file");
			}

			stack_push(lex, c, false);
			usize size = lex_rune(lex, buf);
			buffer_insert(lex, buf, size);

			c = nextchr(lex, NULL, false);
		}

		out->kind = TK_CCONST;
		out->storage = TYPE_STRING;
		out->str.len = lex->buflen;
		out->str.ptr = xstrndup(lex->buf, lex->buflen);

		buffer_clear(lex);
		break;
	case '\'':
		c = nextchr(lex, NULL, false);

		if (c == '\'') {
			push_error(out->loc, "Expected character before closing single-quote");
		}

		stack_push(lex, c, false);
		usize size = lex_rune(lex, buf);
		buf[size] = '\0';

		u8_decode(buf, &out->rune);

		c = nextchr(lex, NULL, false);
		if (c != '\'') {
			push_error(out->loc, "Expected closing single-quote");
		}

		out->kind = TK_CCONST;
		out->storage = TYPE_RUNE;
		break;
	default:
		assert(0); // UNREACHABLE
	}

	return out->kind;
}

static TokenKind lex_duo_operator(LexState *lex, Token *out) {
	u32 c = nextchr(lex, &out->loc, false);
	assert(c != UTF8_EOF);

	switch (c) {
	case '=':
		c = nextchr(lex, NULL, false);
		if (c == '=') {
			out->kind = TK_LEQUAL_EQ;
		} else {
			stack_push(lex, c, false);
			out->kind = TK_EQUAL;
		}
		break;
	case '!':
		c = nextchr(lex, NULL, false);
		if (c == '=') {
			out->kind = TK_LNOT_EQ;
		} else {
			stack_push(lex, c, false);
			out->kind = TK_LNOT;
		}
		break;
	case ':':
		c = nextchr(lex, NULL, false);
		if (c == ':') {
			out->kind = TK_COLON2;
		} else {
			stack_push(lex, c, false);
			out->kind = TK_COLON;
		}
		break;
	case '^':
		c = nextchr(lex, NULL, false);
		if (c == '=') {
			out->kind = TK_BXOR_EQ;
		} else {
			stack_push(lex, c, false);
			out->kind = TK_BXOR;
		}
		break;
	case '*':
		c = nextchr(lex, NULL, false);
		if (c == '=') {
			out->kind = TK_STAR_EQ;
		} else {
			stack_push(lex, c, false);
			out->kind = TK_STAR;
		}
		break;
	case '%':
		c = nextchr(lex, NULL, false);
		if (c == '=') {
			out->kind = TK_MOD_EQ;
		} else {
			stack_push(lex, c, false);
			out->kind = TK_MOD;
		}
		break;
	case '+':
		c = nextchr(lex, NULL, false);
		if (c == '=') {
			out->kind = TK_PLUS_EQ;
		} else {
			stack_push(lex, c, false);
			out->kind = TK_PLUS;
		}
		break;
	case '-':
		c = nextchr(lex, NULL, false);
		if (c == '>') {
			out->kind = TK_ARROW;
		} else if (c == '=') {
			out->kind = TK_MINUS_EQ;
		} else {
			stack_push(lex, c, false);
			out->kind = TK_MINUS;
		}
		break;
	case '&':
		c = nextchr(lex, NULL, false);
		if (c == '&') {
			out->kind = TK_LAND;
		} else if (c == '=') {
			out->kind = TK_BAND_EQ;
		} else {
			stack_push(lex, c, false);
			out->kind = TK_BAND;
		}
		break;
	case '|':
		c = nextchr(lex, NULL, false);
		if (c == '|') {
			out->kind = TK_LOR;
		} else if (c == '=') {
			out->kind = TK_BOR_EQ;
		} else {
			stack_push(lex, c, false);
			out->kind = TK_BOR;
		}
		break;
	case '/':
		c = nextchr(lex, NULL, false);
		if (c == '/') {
			while (c != UTF8_EOF && c != '\n') {
				c = nextchr(lex, NULL, false);
			}
			out->kind = lex_scan(lex, out); // Search for a valid token
		} else if (c == '=') {
			out->kind = TK_SLASH_EQ;
		} else {
			stack_push(lex, c, false);
			out->kind = TK_SLASH;
		}
		break;
	default:
		assert(0); // UNREACHABLE
	}

	return out->kind;
}

static TokenKind lex_tri_operator(LexState *lex, Token *out) {
	u32 c = nextchr(lex, &out->loc, false);
	assert(c != UTF8_EOF);

	// TODO: Improve 3 characters operator parsing
	switch (c) {
	case '<':
		c = nextchr(lex, NULL, false);
		if (c == '<') {
			c = nextchr(lex, NULL, false);
			if (c == '=') {
				out->kind = TK_SHIFTL_EQ;
			} else {
				stack_push(lex, c, false);
				out->kind = TK_SHIFTL;
			}
		} else {
			stack_push(lex, c, false);
			out->kind = TK_LESS;
		}
		break;
	case '>':
		c = nextchr(lex, NULL, false);
		if (c == '>') {
			c = nextchr(lex, NULL, false);
			if (c == '=') {
				out->kind = TK_SHIFTR_EQ;
			} else {
				stack_push(lex, c, false);
				out->kind = TK_SHIFTR;
			}
		} else {
			stack_push(lex, c, false);
			out->kind = TK_GREATER;
		}
		break;
	default:
		assert(0); // UNREACHABLE
	}

	return out->kind;
}

void lex_init(LexState *lex, FILE *file) {
	memset(lex, 0, sizeof(LexState));

	lex->file = file;
	lex->loc.lineno = 1;
	lex->loc.colno = 1;

	lex->bufsize = 128;
	lex->buf = xcalloc(1, lex->bufsize * sizeof(char));

	lex->stack[0] = UTF8_INVALID;
	lex->stack[1] = UTF8_INVALID;
}

void lex_close(LexState *lex) {
	fclose(lex->file);
	free(lex->buf);
}

TokenKind lex_scan(LexState *lex, Token *tok) {
	u32 c = trimspaces(lex, &tok->loc);
	if (c == UTF8_EOF) { // Check if we reached the end-of-file
		tok->kind = TK_EOF;
		return tok->kind;
	}

	// We only scan ASCII characters for identifiers and digits
	if (c <= 0x7f) {
		if (isdigit(c)) {
			stack_push(lex, c, false);
			return lex_number(lex, tok);
		}

		if (isalpha(c) || c == '_') {
			stack_push(lex, c, false);
			return lex_identifier(lex, tok);
		}
	}

	switch (c) {
	case '"':
	case '\'':
		stack_push(lex, c, false);
		return lex_string(lex, tok);
	case '=': // = ==
	case '!': // ! !=
	case ':': // : ::
	case '^': // ^ ^=
	case '*': // * *=
	case '%': // % %=
	case '+': // + +=
	case '-': // - -= ->
	case '&': // & &= &&
	case '|': // | |= ||
	case '/': // / /= //
		stack_push(lex, c, false);
		return lex_duo_operator(lex, tok);
	case '<': // < <= << <<=
	case '>': // > >= >> >>=
		stack_push(lex, c, false);
		return lex_tri_operator(lex, tok);
	case '{':
		tok->kind = TK_BRACE_L;
		break;
	case '}':
		tok->kind = TK_BRACE_R;
		break;
	case '[':
		tok->kind = TK_BRACKET_L;
		break;
	case ']':
		tok->kind = TK_BRACKET_R;
		break;
	case '~':
		tok->kind = TK_BNOT;
		break;
	case ',':
		tok->kind = TK_COMMA;
		break;
	case '.':
		tok->kind = TK_DOT;
		break;
	case '(':
		tok->kind = TK_PAREN_L;
		break;
	case ')':
		tok->kind = TK_PAREN_R;
		break;
	case ';':
		tok->kind = TK_SEMICOLON;
		break;
	default:
		push_error(lex->loc, "Unknown symbol found: %c", c);
		break;
	}

	return tok->kind;
}

const char *lex_tok2str(TokenKind tok) {
	assert(tok <= TK_LAST_OPERATOR);
	return tokens[tok];
}
