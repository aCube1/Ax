#ifndef _AX_UTIL_H_
#define _AX_UTIL_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum LogLevel {
	LOG_TRACE,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_FATAL,
};

#define log_trace(...) log_message(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_message(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_message(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_message(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_message(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_message(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

/*!
 * Print formatted string to the STDERR.
 *
 * @param level Message debugging level.
 * @param file  File which the function has been called.
 * @param line  File line which the function has been called.
 * @param fmt   Formatted string to be displayed.
 */
void log_message(int level, const char *file, int line, const char *fmt, ...);

void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrndup(const char *str, size_t len);

FILE *xfopen(const char *filename, const char *modes);

#endif
