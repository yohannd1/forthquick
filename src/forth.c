#include "ArrayList.h"
#include "forth.h"
#include "log.h"

#include <stdlib.h>

#define max(a, b) ((a > b) ? a : b)

static bool f_Stack_alloc(f_Stack *f) {
	size_t newsize = sizeof(f_Int) * max(f->len, f->capacity);

	f_Int *newbuf = (f->buf == NULL) ? malloc(newsize) : realloc(f->buf, newsize);

	if (newbuf == NULL) return false;

	f->buf = newbuf;
	return true;
}

static bool f_tryParseInt(SliceConst s, f_Int *it) {
	f_Int mul = 1;
	f_Int result = 0;

	size_t i = 0;
	for (; i < s.len; i++) {
		char c = s.ptr[s.len - 1 - i];
		if (c < '0' || c > '9') return false;
		result += (f_Int) (c - '0') * mul;
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

bool f_Stack_push(f_Stack *f, f_Int i) {
	f->len++;
	if (!f_Stack_alloc(f)) {
		f->len--;
		return false;
	}

	f->buf[f->len - 1] = i;
	return true;
}

bool f_Stack_pop(f_Stack *f, f_Int *dest) {
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
	dest->reader_str = (SliceConst) { .ptr = NULL, .len = 0 };
	dest->reader_idx = 0;
	dest->prompt_string = "$ ";
	dest->echo = false;

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

void f_State_compileAndRun(f_State *s, SliceConst line) {
	if (s->echo) fprintf(stderr, "\n%s%.*s\n", s->prompt_string, (int) line.len, line.ptr);

	SliceMut bytecode;
	if (!f_State_compile(s, line, &bytecode)) return;
	f_State_run(s, SLICE_MUT2CONST(bytecode));
}

bool f_State_compile(f_State *s, SliceConst line, SliceMut *dest) {
	s->reader_str = line;
	s->reader_idx = 0;

	ArrayList b = ArrayList_init();

	SliceConst w;
	while ((w = f_State_getToken(s)).ptr != NULL) {
		Dict_Entry *e = Dict_findN(&s->words, w);
		if (e != NULL) {
			/* process word */
			f_Word *w = e->value;

			/* TODO: if immediate, run immediately duh */

			ArrayList_push(&b, F_INS_CALLWORD);

			/* decode pointer into an array */
			size_t ptr_int = (size_t) w->func;
			/* logD("ENCODING PTR %ld", ptr_int); */
			size_t i;
			for (i = sizeof(size_t); i > 0; i--) {
				ArrayList_push(&b, (size_t) (ptr_int >> (i * 8 - 8)));
			}

			continue;
		}

		f_Int it;
		if (f_tryParseInt(w, &it)) {
			/* process int */
			/* TODO: error handling? */
			ArrayList_push(&b, F_INS_PUSHINT);

			/* logD("ENCODING INT %d", it); */
			size_t i;
			for (i = sizeof(f_Int); i > 0; i--) {
				uint8_t val = it >> (i * 8 - 8);
				/* logD("-> %d", val); */
				ArrayList_push(&b, val);
			}
			continue;
		}

		fprintf(stderr, "%.*s? ", (int) w.len, w.ptr);
		goto error;
	}

	*dest = b.items;
	return true;

error:
	ArrayList_deinit(&b);
	return false;
}

bool f_State_run(f_State *s, SliceConst bytecode) {
	size_t i = 0;
	for (; i < bytecode.len; i++) {
		uint8_t ins = bytecode.ptr[i];
		switch (ins) {
		case F_INS_PUSHINT: {
			f_Int it = 0;

			/* logD("DECODING INT..."); */
			size_t j;
			for (j = sizeof(f_Int); j > 0; j--) {
				if (i >= bytecode.len) {
					/* logD("bytecode abruptly ended"); */
					return false;
				}

				uint8_t byte = bytecode.ptr[++i];
				/* logD("+= %d << %d", byte, (j * 8 - 8)); */
				it += (f_Int) byte << (j * 8 - 8);
			}
			/* logD("DECODED INT %d", it); */
			f_Stack_push(&s->working_stack, it);
			break;
		}
		case F_INS_CALLWORD: {
			size_t it = 0;

			/* logD("DECODING PTR..."); */
			size_t j;
			for (j = sizeof(size_t); j > 0; j--) {
				if (i >= bytecode.len) {
					/* logD("bytecode abruptly ended"); */
					return false;
				}

				uint8_t byte = bytecode.ptr[++i];
				/* logD("+= %d << %d", byte, (j * 8 - 8)); */
				it += (size_t) byte << (j * 8 - 8);
			}

			/* logD("DECODED PTR %d", it); */
			f_WordFunc f = (f_WordFunc) it;
			if (!f(s)) return false;
		}
		}
	}
	return true;
}

static bool isWhitespace(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

SliceConst f_State_getToken(f_State *s) {
	/* skip leading whitespace */
	while (s->reader_idx < s->reader_str.len) {
		char c = s->reader_str.ptr[s->reader_idx];
		if (!isWhitespace(c)) break;
		s->reader_idx++;
	}

	if (s->reader_idx >= s->reader_str.len) return (SliceConst) { .ptr = NULL, .len = 0 };

	const uint8_t *ret_start = &s->reader_str.ptr[s->reader_idx];
	size_t i = 0;
	while (true) {
		char c = s->reader_str.ptr[s->reader_idx];
		if (isWhitespace(c) || c == '\0') break;

		s->reader_idx++;
		i++;
	}

	if (i > 0) return (SliceConst) { .ptr = ret_start, .len = i };
	return (SliceConst) { .ptr = NULL, .len = 0 };
}
