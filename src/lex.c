#include "lex.h"

#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>

#define CEOF '\0'

static _Noreturn void push_error(Location loc, const char *fmt, ...);

static TokenKind lex_number(LexState *lex, Token *out);

static void buffer_insert(LexState *lex, const char *s, size_t len);
static void buffer_clear(LexState *lex);

static void stack_push(LexState *lex, char c, bool frombuf);

static char nextchr(LexState *lex, Location *loc, bool buffer);
static char trimspaces(LexState *lex, Location *loc);

void lex_init(LexState *lex, FILE *file) {
	memset(lex, 0, sizeof(LexState));

	lex->file = file;
	lex->loc.lineno = 1;
	lex->loc.colno = 1;

	lex->bufsize = 128;
	lex->buf = xcalloc(1, lex->bufsize * sizeof(char));
}

void lex_close(LexState *lex) {
	fclose(lex->file);
	free(lex->buf);
}

TokenKind lex_scan(LexState *lex, Token *out) {
	char c = trimspaces(lex, &out->loc);
	if (c == CEOF) { // Check if we reached the end-of-file
		out->kind = TK_EOF;
		return out->kind;
	}

	if (isdigit(c)) {
		stack_push(lex, c, false);
		return lex_number(lex, out);
	}

	return out->kind;
}

static _Noreturn void push_error(Location loc, const char *fmt, ...) {
	fprintf(stderr, "%d:%d ", loc.lineno, loc.colno);

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");

	exit(EXIT_FAILURE);
}

static TokenKind lex_number(LexState *lex, Token *out) {
	// TODO: Improve number lexing, and storage detection

	enum Base { BIN, OCT, HEX, DEC };

	static const char numbers[][24] = {
		[BIN] = "01",
		[OCT] = "01234567",
		[HEX] = "0123456789abcdefABCDEF",
		[DEC] = "0123456789",
	};

	char c = nextchr(lex, &out->loc, true);
	assert(c != CEOF && isdigit(c));

	enum Base state = DEC;
	int base = 10;

	if (c == '0') {
		c = nextchr(lex, NULL, true);

		if (isdigit(c) || c == '_') {
			push_error(out->loc, "Leading zero in number");
		} else if (c == 'b') { // Binary literal
			state = BIN;
			base = 2;
		} else if (c == 'o') { // Octal literal
			state = OCT;
			base = 8;
		} else if (c == 'x') { // Hexadecimal literal
			state = HEX;
			base = 16;
		}
	}

	if (state != DEC) {
		c = nextchr(lex, NULL, true);
	}

	while (strchr(numbers[state], c)) {
		c = nextchr(lex, NULL, true);
	}

	out->kind = TK_NUMBER;

	errno = 0;
	out->uval = strtoumax(lex->buf + (base == 10 ? 0 : 2), NULL, base);
	if (errno == ERANGE) {
		push_error(out->loc, "Integer constant overflow");
	}

	buffer_clear(lex);
	return TK_NUMBER;
}

static void buffer_insert(LexState *lex, const char *s, size_t len) {
	if (lex->buflen + len > lex->bufsize) {
		lex->bufsize *= 2;
		lex->buf = xrealloc(lex->buf, lex->bufsize);
	}

	memcpy(lex->buf + lex->buflen, s, len);
	lex->buflen += len;
	lex->buf[lex->buflen] = '\0';
}

static void buffer_clear(LexState *lex) {
	lex->buflen = 0;
	lex->buf[0] = '\0';
}

static void stack_push(LexState *lex, char c, bool frombuf) {
	assert(lex->stack[1] == CEOF); // This should never happen

	lex->stack[1] = lex->stack[0];
	lex->stack[0] = c;

	if (frombuf) { // Consume the character from buffer
		lex->buflen -= 1;
		lex->buf[lex->buflen] = '\0';
	}
}

static void update_line(Location *loc, char c) {
	if (c == '\n') { // Update line number and reset column
		loc->lineno += 1;
		loc->colno = 0;
	} else if (c == '\t') {
		loc->colno += 4; // All tabs count as 4 columns
	} else {
		loc->colno += 1;
	}
}

static char nextchr(LexState *lex, Location *loc, bool buffer) {
	char c;

	if (lex->stack[0] != CEOF) {
		c = lex->stack[0];
		lex->stack[0] = lex->stack[1];
		lex->stack[1] = CEOF;
	} else {
		c = (char)fgetc(lex->file);

		if (feof(lex->file)) {
			c = CEOF;
		}
	}

	update_line(&lex->loc, c);
	if (loc != NULL) {
		*loc = lex->loc;
	}

	// Check if we need to store the character in the buffer
	if (buffer) {
		buffer_insert(lex, &c, 1);
	}

	return c;
}

static inline bool is_space(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

static char trimspaces(LexState *lex, Location *loc) {
	char c = ' ';

	while (c != CEOF && is_space(c)) {
		c = nextchr(lex, loc, false);
	}

	return c;
}
