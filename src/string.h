#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

typedef struct StringView {
	const char *ptr;
	size_t len;
} StringView;

typedef struct StringViewM {
	char *ptr;
	size_t len;
} StringViewM;

#endif
