#include "log.h"

void logD(const char *fmt, ...) {
	va_list args;

	fprintf(stderr, "[log] ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void die(const char *fmt, ...) {
	va_list args;

	fprintf(stderr, "[fatal] ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(1);
}
