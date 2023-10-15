#include "ArrayList.h"
#include "forth.h"
#include "log.h"

#include <stdlib.h>

#define max(a, b) ((a > b) ? a : b)

static bool f_State_evalWord(f_State *s, const f_Word *w);

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

static bool f_tryParseFloat(SliceConst s, f_Int *it) {
	/* i feel so goddamn insane */
	char *mem = malloc(s.len + 1);
	if (!mem) die("OOM");
	memcpy(mem, s.ptr, s.len);
	mem[s.len] = '\0';

	float f;
	bool worked = sscanf(mem, "%f", &f) == 1;
	if (worked) *it = f_floatToInt(f);

	free(mem);
	return worked;
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
	if (!Dict_init(&dest->variables, 128)) return false;
	dest->bytecode = NULL;
	dest->is_closed = false;
	dest->reader_str = (SliceConst) { .ptr = NULL, .len = 0 };
	dest->reader_idx = 0;
	dest->prompt_string = "$ ";
	dest->echo = false;

	return true;
}

void f_State_defineWord(f_State *s, const char *wordname, f_WordFunc func, bool is_immediate) {
	f_Word w;
	w.is_immediate = is_immediate;
	w.tag = F_WORDT_FUNC;
	w.un.func_ptr = func;

	f_Word *mem = malloc(sizeof(f_Word));
	if (mem == NULL) die("OOM");
	*mem = w;
	Dict_put(&s->words, SLICE_FROMNUL(wordname), mem);
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
	s->bytecode = &b;

	SliceConst w;
	while ((w = f_State_getToken(s)).ptr != NULL) {
		if (!f_State_compileSingleWord(s, &b, w)) goto error;
	}

	s->bytecode = NULL;
	*dest = b.items;
	return true;

error:
	s->bytecode = NULL;
	ArrayList_deinit(&b);
	return false;
}

bool f_State_compileSingleWord(f_State *s, ArrayList *b, SliceConst w) {
	Dict_Entry *e = Dict_findN(&s->words, w);
	if (e != NULL) {
		/* process word */
		f_Word *w = e->value;

		if (w->is_immediate) {
			return f_State_evalWord(s, w);
		}
		ArrayList_push(b, F_INS_CALLWORD);

		/* decode pointer into an array */
		size_t ptr_int = (size_t) w;
		/* logD("ENCODING PTR %ld", ptr_int); */
		size_t i;
		for (i = sizeof(size_t); i > 0; i--) {
			ArrayList_push(b, (size_t) (ptr_int >> (i * 8 - 8)));
		}

		return true;
	}

	bool int_ok = false;

	/* try parsing int/float and push it into the stack */
	f_Int it;
	int_ok = f_tryParseInt(w, &it);
	if (!int_ok) int_ok = f_tryParseFloat(w, &it);

	if (int_ok) {
		/* TODO: error handling? */
		ArrayList_push(b, F_INS_PUSHINT);

		size_t i;
		for (i = sizeof(f_Int); i > 0; i--) {
			uint8_t val = it >> (i * 8 - 8);
			ArrayList_push(b, val);
		}

		return true;
	}

	fprintf(stderr, "%.*s? ", (int) w.len, w.ptr);
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
			f_Word *w = (f_Word *) it;
			if (!f_State_evalWord(s, w)) return false;
			break;
		}
		case F_INS_PREAD: {
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
			return f_Stack_push(&s->working_stack, *(f_Int*)it);
		}
		case F_INS_PWRITE: {
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

			f_Int n;
			if (!f_Stack_pop(&s->working_stack, &n)) {
				logD("StackUnderflow");
				return false;
			}

			f_Int *ptr = (f_Int *) it;
			*ptr = n;
			break;
		}
		case F_INS_JMPCOND: {
			size_t it = 0;

			/* logD("DECODING LEN..."); */
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
			/* logD("DECODED LEN %d", it); */

			f_Int n;
			if (!f_Stack_pop(&s->working_stack, &n)) {
				logD("StackUnderflow");
				return false;
			}

			if (n == 0) i += it;
			break;
		}
		}
	}
	return true;
}

static bool f_State_evalWord(f_State *s, const f_Word *w) {
	if (w->tag == F_WORDT_FUNC) return w->un.func_ptr(s);

	if (w->tag == F_WORDT_BYTECODE)
		return f_State_run(s, SLICE_MUT2CONST(w->un.bytecode.items));

	logD("unknown WORDT tag #%02x", w->tag);
	return false;
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

f_Int f_floatToInt(float f) {
	union {
		float f;
		f_Int i;
	} u;
	u.f = f;
	return u.i;
}

float f_intToFloat(f_Int i) {
	union {
		float f;
		f_Int i;
	} u;
	u.i = i;
	return u.f;
}
