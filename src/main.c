#include "lex.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
	if (argc != 2) {
		log_fatal("Usage: %s <file.ax>", argv[0]);
		return EXIT_FAILURE;
	}

	// Enforce extension
	const char *ext = strrchr(argv[1], '.');
	if (ext == NULL || (strcmp(ext, ".ax") != 0 && strcmp(ext, ".AX") != 0)) {
		log_fatal(
			"Unknown extension found in file: %s! Valid extensions are: .ax .AX", argv[1]
		);
		return EXIT_FAILURE;
	}

	FILE *file = xfopen(argv[1], "rb");

	LexState lex = { 0 };
	lex_init(&lex, file);

	Token tok = { 0 };
	while (lex_scan(&lex, &tok) != TK_EOF) {
		if (tok.kind == TK_NUMBER) {
			log_debug("%d:%d > %d", tok.loc.lineno, tok.loc.colno, tok.uval);
		}

		tok.kind = TK_NONE;
	}

	lex_close(&lex);
	return EXIT_SUCCESS;
}
