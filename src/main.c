#include "lex.h"
#include "utf8.h"
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
		if (tok.kind <= TK_LAST_OPERATOR) {
			log_debug(
				"%d:%d -> %s", tok.loc.lineno, tok.loc.colno, lex_tok2str(tok.kind)
			);
		}

		if (tok.kind == TK_IDENTIFIER && tok.ident != NULL) {
			log_debug("%d:%d -> %s", tok.loc.lineno, tok.loc.colno, tok.ident);
			free(tok.ident);
			tok.ident = NULL;
		}

		if (tok.kind == TK_CCONST) {
			switch (tok.storage) {
			case TYPE_FLOAT:
				log_debug("%d:%d -> %f", tok.loc.lineno, tok.loc.colno, tok.fval);
				break;
			case TYPE_INT:
				log_debug("%d:%d -> %d", tok.loc.lineno, tok.loc.colno, tok.uval);
				break;
			case TYPE_STRING:
				if (tok.str.ptr != NULL) {
					log_debug("%d:%d -> %s", tok.loc.lineno, tok.loc.colno, tok.str.ptr);
					free(tok.str.ptr);
					tok.str.ptr = NULL;
				}
				break;
			case TYPE_RUNE:
				log_debug("%d:%d -> %c", tok.loc.lineno, tok.loc.colno, tok.rune);
				break;
			default:
				break;
			}
		}
	}

	lex_close(&lex);
	return EXIT_SUCCESS;
}
