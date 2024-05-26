#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void log_message(int level, const char *file, int line, const char *fmt, ...) {
	static const char *level_names[] = {
		"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL",
	};
	static const char *level_color[] = {
		"\x1b[34m", "\x1b[32m", "\x1b[36m", "\x1b[33m", "\x1b[31m", "\x1b[35m",
	};

	time_t timer = time(NULL);
	struct tm *ltime = localtime(&timer);

	char buf[16] = { 0 };
	buf[strftime(buf, 16, "%H:%M:%S", ltime)] = '\0';

#ifdef _DEBUG
	fprintf(
		stderr, "%s %s[%s]\x1b[0m \x1b[90m%s:%d\x1b[0m - ", buf, level_color[level],
		level_names[level], file, line
	);
#else
	fprintf(stderr, "%s %s[%s]\x1b[0m - ", buf, level_color[level], level_names[level]);
#endif

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "%s", "\n");
	va_end(args);
}
