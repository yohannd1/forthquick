#include "log.h"

void logD(const char *fmt, ...) {
	fprintf(stderr, "[log] ");
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void die(const char *fmt, ...) {
	fprintf(stderr, "[fatal] ");
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(1);
}
