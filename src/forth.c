#include "forth.h"
#include "string.h"

#include <stdio.h> /* FIXME: only for debug */
#include <stdlib.h>

#define max(a, b) ((a > b) ? a : b)

static bool f_Stack_alloc(f_Stack *f) {
	size_t newsize = sizeof(f_Item) * max(f->len, f->capacity);

	f_Item *newbuf = (f->buf == NULL) ? malloc(newsize) : realloc(f->buf, newsize);

	if (newbuf == NULL) return false;

	f->buf = newbuf;
	return true;
}

static bool f_tryParseItem(StringView s, f_Item *it) {
	f_Item mul = 1;
	f_Item result = 0;

	size_t i = 0;
	for (; i < s.len; i++) {
		char c = s.ptr[s.len - 1 - i];
		if (c < '0' || c > '9') return false;
		result += (f_Item)(c - '0') * mul;
		mul *= 10;
	}

	*it = result;
	return true;
}

bool f_Stack_init(f_Stack *f) {
	f->buf = NULL;
	f->len = 0;
	f->capacity = 128;
	if (!f_Stack_alloc(f)) return false;

	return true;
}

bool f_Stack_push(f_Stack *f, f_Item i) {
	f->len++;
	if (!f_Stack_alloc(f)) {
		f->len--;
		return false;
	}

	f->buf[f->len - 1] = i;
	return true;
}

bool f_Stack_pop(f_Stack *f, f_Item *dest) {
	if (f->len == 0) return false;
	*dest = f->buf[f->len - 1];
	f->len--;
	return true;
	/* TODO: decide when to realloc */
}

bool f_State_init(f_State *dest) {
	if (!f_Stack_init(&dest->working_stack)) return false;
	if (!f_Stack_init(&dest->return_stack)) return false;
	if (!Dict_init(&dest->words, 128)) return false;
	dest->is_closed = false;
	dest->reader_str = NULL;
	dest->reader_idx = 0;
	dest->prompt_string = "$ ";

	return true;
}

void f_State_defineWord(f_State *s, const char *wordname, f_WordFunc func, bool is_immediate) {
	f_Word *mem = malloc(sizeof(f_Word));
	if (mem == NULL) {
		fprintf(stderr, "OOM\n");
		exit(1);
	}
	*mem = (f_Word) { .func = func, .is_immediate = is_immediate };
	Dict_put(&s->words, wordname, mem);
}

void f_State_evalString(f_State *s, const char *line, bool echo) {
	if (echo) fprintf(stderr, "%s%s\n", s->prompt_string, line);
	s->reader_str = line;
	s->reader_idx = 0;

	StringView w;
	while ((w = f_State_getToken(s)).ptr != NULL) {
		Dict_Entry *e = Dict_findN(&s->words, w);
		if (e != NULL) {
			f_Word *w = e->value;
			if (!w->func(s)) break;
			continue;
		}

		f_Item it;
		if (f_tryParseItem(w, &it)) {
			f_Stack_push(&s->working_stack, it);
			continue;
		}

		fprintf(stderr, "%.*s? ", (int)w.len, w.ptr);
		break;
	}
	fprintf(stderr, "\n");
}

static bool isWhitespace(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

StringView f_State_getToken(f_State *s) {
	/* skip leading whitespace */
	for (; isWhitespace(s->reader_str[s->reader_idx]); s->reader_idx++)
		;

	if (s->reader_str[s->reader_idx] == '\0') return (StringView) { .ptr = NULL, .len = 0 };

	const char *ret_start = &s->reader_str[s->reader_idx];
	size_t i = 0;
	while (true) {
		char c = s->reader_str[s->reader_idx];
		if (isWhitespace(c) || c == '\0') break;

		s->reader_idx++;
		i++;
	}

	if (i > 0) return (StringView) { .ptr = ret_start, .len = i };
	return (StringView) { .ptr = NULL, .len = 0 };
}
