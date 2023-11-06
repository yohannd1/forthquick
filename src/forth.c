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
	char *mem;
	bool worked;
	float f;

	/* i feel so goddamn insane */
	mem = malloc(s.len + 1);
	if (mem == 0) die("OOM");
	memcpy(mem, s.ptr, s.len);
	mem[s.len] = '\0';

	worked = sscanf(mem, "%f", &f) == 1;
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
	dest->bytecode = NULL;
	dest->next_byte = NULL;
	dest->is_closed = false;
	dest->reader_str.ptr = NULL;
	dest->reader_str.len = 0;
	dest->reader_idx = 0;
	dest->prompt_string = "$ ";
	dest->echo = false;

	return true;
}

void f_State_defineWord(f_State *s, const char *wordname, f_WordFunc func, bool is_immediate) {
	f_Word w;
	f_Word *mem;

	w.is_immediate = is_immediate;
	w.tag = F_WORDT_FUNC;
	w.un.func_ptr = func;

	mem = malloc(sizeof(f_Word));
	if (mem == NULL) die("OOM");
	*mem = w;
	Dict_put(&s->words, SliceConst_fromCString(wordname), mem);
}

void f_State_compileAndRun(f_State *s, SliceConst line) {
	SliceMut bytecode;

	if (s->echo) fprintf(stderr, "%s%.*s\n", s->prompt_string, (int) line.len, line.ptr);

	if (!f_State_compile(s, line, &bytecode)) return;
	f_State_run(s, SliceConst_fromMut(bytecode));
}

bool f_State_compile(f_State *s, SliceConst line, SliceMut *dest) {
	ArrayList b;
	SliceConst w;

	s->reader_str = line;
	s->reader_idx = 0;

	b = ArrayList_init();
	s->bytecode = &b;

	while ((w = f_State_getToken(s)).ptr != NULL) {
		if (!f_State_compileSingleWord(s, &b, w, 0)) goto error;
	}

	ArrayList_push(&b, F_INS_RETURN);

	s->bytecode = NULL;
	*dest = b.items;
	return true;

error:
	s->bytecode = NULL;
	ArrayList_deinit(&b);
	return false;
}

bool f_State_compileSingleWord(f_State *s, ArrayList *b, SliceConst w, int jump) {
	bool int_ok = false;
	Dict_Entry *e;
	f_Int it;

	e = Dict_findN(&s->words, w);
	if (e != NULL) {
		size_t ptr_int, i;
		f_Word *w;

		/* process word */
		w = e->value;

		if (w->is_immediate) {
			return f_State_evalWord(s, w);
		}
		ArrayList_push(b, jump ? F_INS_JMPWORD : F_INS_CALLWORD);

		ptr_int = (size_t) w;
		for (i = sizeof(size_t); i > 0; i--) {
			ArrayList_push(b, (size_t) (ptr_int >> (i * 8 - 8)));
		}

		return true;
	}

	/* try parsing int/float and push it into the stack */
	int_ok = f_tryParseInt(w, &it);
	if (!int_ok) int_ok = f_tryParseFloat(w, &it);

	if (int_ok) {
		size_t i;

		/* TODO: error handling? */
		ArrayList_push(b, F_INS_PUSHINT);

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
	s->next_byte = bytecode.ptr;

	for (;;) {
		uint8_t ins = *(s->next_byte++);
		switch (ins) {
		case F_INS_RETURN: {
			f_Int i;

			if (s->return_stack.len == 0) return true; /* finish gracefully if the return stack is empty */

			f_Stack_pop(&s->return_stack, &i);
			logD("Shall be returning to %lu", i);
			s->next_byte = (uint8_t*)i;
			break;
		}
		case F_INS_PUSHINT: {
			f_Int it = 0;
			size_t j;

			for (j = sizeof(f_Int); j > 0; j--) {
				uint8_t byte;

				byte = *(s->next_byte++);
				it += (f_Int) byte << (j * 8 - 8);
			}
			f_Stack_push(&s->working_stack, it);
			break;
		}
		case F_INS_CALLWORD: {
			size_t it = 0, j;
			f_Word *w;
			f_Int tmp;

			for (j = sizeof(size_t); j > 0; j--) {
				uint8_t byte;

				byte = *(s->next_byte++);
				it += (size_t) byte << (j * 8 - 8);
			}

			w = (f_Word *) it;
			switch (w->tag) {
			case F_WORDT_FUNC:
				/* call the C function */
				if (!w->un.func_ptr(s)) return false;
				break;
			case F_WORDT_BYTECODE:
				/* push current bytecode addr and jump to the word bytecode */
				tmp = (f_Int)s->next_byte;
				f_Stack_push(&s->return_stack, tmp);
				s->next_byte = w->un.bytecode.items.ptr;
				break;
			default:
				logD("unknown WORDT tag #%02x", w->tag);
				return false;
			}

			break;
		}
		case F_INS_PREAD: {
			size_t it, j;

			it = 0;
			for (j = sizeof(size_t); j > 0; j--) {
				uint8_t byte;

				byte = *(s->next_byte++);
				it += (size_t) byte << (j * 8 - 8);
			}

			return f_Stack_push(&s->working_stack, *(f_Int*)it);
		}
		case F_INS_PWRITE: {
			size_t it, j;
			f_Int n;

			it = 0 ;
			for (j = sizeof(size_t); j > 0; j--) {
				uint8_t byte;

				byte = *(s->next_byte++);
				it += (size_t) byte << (j * 8 - 8);
			}

			if (!f_Stack_pop(&s->working_stack, &n)) {
				logD("StackUnderflow");
				return false;
			}

			*((f_Int *) it) = n;
			break;
		}
		case F_INS_JMPCOND: {
			size_t it, j;
			f_Int n;

			it = 0;
			for (j = sizeof(size_t); j > 0; j--) {
				uint8_t byte;

				byte = *(s->next_byte++);
				it += (size_t) byte << (j * 8 - 8);
			}

			if (!f_Stack_pop(&s->working_stack, &n)) {
				logD("StackUnderflow");
				return false;
			}

			if (n == 0) s->next_byte += it;
			break;
		}
		default:
			logD("unknown instruction #%02x", ins);
			return false;
		}
	}
	return true;
}

static bool f_State_evalWord(f_State *s, const f_Word *w) {
	if (w->tag == F_WORDT_FUNC) return w->un.func_ptr(s);

	if (w->tag == F_WORDT_BYTECODE) die("attempt to run bytecode function at compile time"); /* TODO: this should work! */

	logD("unknown WORDT tag #%02x", w->tag);
	return false;
}

static bool isWhitespace(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

SliceConst f_State_getToken(f_State *s) {
	size_t i;
	SliceConst r;
	const uint8_t *ret_start;

	/* skip leading whitespace */
	while (s->reader_idx < s->reader_str.len) {
		char c = s->reader_str.ptr[s->reader_idx];
		if (!isWhitespace(c)) break;
		s->reader_idx++;
	}

	if (s->reader_idx >= s->reader_str.len) return SliceConst_newEmpty();
	

	ret_start = &s->reader_str.ptr[s->reader_idx];
	for (i = 0;; s->reader_idx++, i++) {
		char c = s->reader_str.ptr[s->reader_idx];
		if (isWhitespace(c) || c == '\0') break;
	}

	if (i > 0) {
		r.ptr = ret_start;
		r.len = i;
		return r;
	}

	return SliceConst_newEmpty();
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
