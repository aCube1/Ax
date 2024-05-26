#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
	if (argc != 2) {
		log_fatal("Usage: %s <file.ax>", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Enforce extension
	const char *ext = strrchr(argv[1], '.');
	if (ext == NULL || (strncmp(ext, ".ax", 3) != 0 && strncmp(ext, ".AX", 3) != 0)) {
		log_fatal("Unknown extension found! Valid extensions are: .ax .AX");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
