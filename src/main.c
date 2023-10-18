#include "dict.h"
#include "forth.h"
#include "log.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAS_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

char *promptReadLine(f_State *s);

bool fw_add(f_State *s);
bool fw_sub(f_State *s);
bool fw_mul(f_State *s);
bool fw_div(f_State *s);
bool fw_print(f_State *s);
bool fw_lt(f_State *s);
bool fw_gt(f_State *s);
bool fw_lte(f_State *s);
bool fw_gte(f_State *s);

bool fw_fadd(f_State *s);
bool fw_fsub(f_State *s);
bool fw_fmul(f_State *s);
bool fw_fdiv(f_State *s);
bool fw_fprint(f_State *s);
bool fw_flt(f_State *s);
bool fw_fgt(f_State *s);
bool fw_flte(f_State *s);
bool fw_fgte(f_State *s);

bool fw_words(f_State *s);
bool fw_show(f_State *s);
bool fw_quit(f_State *s);
bool fw_beginWordCompile(f_State *s);
bool fw_beginComment(f_State *s);
bool fw_beginComment2(f_State *s);
bool fw_fetch(f_State *s);
bool fw_store(f_State *s);
bool fw_defVar(f_State *s);
/* bool fw_toVar(f_State *s); */
/* bool fw_fromVar(f_State *s); */
bool fw_beginIf(f_State *s);

void defWords(f_State *s) {
	/* memory primitives */
	f_State_defineWord(s, "@", fw_fetch, false);
	f_State_defineWord(s, "!", fw_store, false);
	f_State_defineWord(s, "var:", fw_defVar, true);

	/* int stuff */
	f_State_defineWord(s, "+", fw_add, false);
	f_State_defineWord(s, "-", fw_sub, false);
	f_State_defineWord(s, "*", fw_mul, false);
	f_State_defineWord(s, "/", fw_div, false);
	f_State_defineWord(s, "<", fw_lt, false);
	f_State_defineWord(s, ">", fw_gt, false);
	f_State_defineWord(s, "<=", fw_lte, false);
	f_State_defineWord(s, ">=", fw_gte, false);
	f_State_defineWord(s, ".", fw_print, false);

	/* float stuff */
	f_State_defineWord(s, "f+", fw_fadd, false);
	f_State_defineWord(s, "f-", fw_fsub, false);
	f_State_defineWord(s, "f*", fw_fmul, false);
	f_State_defineWord(s, "f/", fw_fdiv, false);
	f_State_defineWord(s, "f<", fw_flt, false);
	f_State_defineWord(s, "f>", fw_fgt, false);
	f_State_defineWord(s, "f<=", fw_flte, false);
	f_State_defineWord(s, "f>=", fw_fgte, false);
	f_State_defineWord(s, "f.", fw_fprint, false);

	f_State_defineWord(s, "words", fw_words, false);
	f_State_defineWord(s, ".s", fw_show, false);
	f_State_defineWord(s, "quit", fw_quit, false);
	f_State_defineWord(s, ":", fw_beginWordCompile, true);
	f_State_defineWord(s, "(", fw_beginComment, true);
	f_State_defineWord(s, "((", fw_beginComment2, true);
	f_State_defineWord(s, "if(", fw_beginIf, true);
}

int main(void) {
	f_State s;
	size_t linelen;
	char *linebuf;


	if (!f_State_init(&s)) die("failed to init state");
	defWords(&s);

	s.echo = true;
	f_State_compileAndRun(&s, SliceConst_fromCString("( Welcome to QuickForth v0.3. )"));
	f_State_compileAndRun(&s, SliceConst_fromCString("( Type 'words' to list words. )"));
	s.echo = false;

	linelen = 128;
	linebuf = malloc(linelen);
	if (linebuf == NULL) printf("OOM\n");
	while (!s.is_closed) {
		char *line;

		fprintf(stderr, "ok\n");
		line = promptReadLine(&s);
		if (line == NULL) {
			s.is_closed = true;
			goto read_end;
		}

		f_State_compileAndRun(&s, SliceConst_fromCString(line));
read_end:
		free(line);
	}

	/* f_State_deinit(&s); */

	return 0;
}

char *promptReadLine(f_State *s) {
#ifdef HAS_READLINE
	char *mem = readline(s->prompt_string);
	if (mem == NULL) return NULL;
	add_history(mem);
#else
	fprintf(stderr, "%s", s->prompt_string);

	const size_t coarse_amount = 128;
	size_t bufsize = coarse_amount;
	char *mem = malloc(bufsize);
	if (mem == NULL) {
		logD("OOM");
		return NULL;
	}

	size_t i = 0;
	for (;; i++) {
		if (i >= bufsize) {
			bufsize += coarse_amount;
			char *new = realloc(mem, bufsize);
			if (new == NULL) {
				logD("OOM");
				free(mem);
				return NULL;
			}
		}

		int c = fgetc(stdin);
		if (c == '\n') break;
		if (c == EOF) {
			free(mem);
			return NULL;
		}
		mem[i] = (char) c;
	}
	mem[i] = '\0';
#endif

	return mem;
}

#define LOG(msg) \
{ fprintf(stderr, "%s", msg); }

#define TRY_POP(var) \
	if (!f_Stack_pop(&s->working_stack, var)) { \
		LOG("StackUnderflow "); \
		return false; \
	}

#define TRY_PUSH(val) \
	if (!f_Stack_push(&s->working_stack, val)) { \
		LOG("OOM "); \
		return false; \
	}

#define DEFWORD(name, statements) bool name(f_State *s) statements
#define DEF_BIN_ARITH_WORD(name, op) \
	DEFWORD(name, { \
		f_Int n1; \
		f_Int n2; \
		TRY_POP(&n2); \
		TRY_POP(&n1); \
		TRY_PUSH(op); \
		return true; \
		}) \

DEF_BIN_ARITH_WORD(fw_add, n1 + n2)
DEF_BIN_ARITH_WORD(fw_sub, n1 - n2)
DEF_BIN_ARITH_WORD(fw_mul, n1 * n2)
DEF_BIN_ARITH_WORD(fw_div, n1 / n2)

DEF_BIN_ARITH_WORD(fw_lt, n1 < n2)
DEF_BIN_ARITH_WORD(fw_gt, n1 > n2)
DEF_BIN_ARITH_WORD(fw_lte, n1 <= n2)
DEF_BIN_ARITH_WORD(fw_gte, n1 >= n2)

DEF_BIN_ARITH_WORD(fw_fadd, f_floatToInt(f_intToFloat(n1) + f_intToFloat(n2)))
DEF_BIN_ARITH_WORD(fw_fsub, f_floatToInt(f_intToFloat(n1) - f_intToFloat(n2)))
DEF_BIN_ARITH_WORD(fw_fmul, f_floatToInt(f_intToFloat(n1) * f_intToFloat(n2)))
DEF_BIN_ARITH_WORD(fw_fdiv, f_floatToInt(f_intToFloat(n1) / f_intToFloat(n2)))

DEF_BIN_ARITH_WORD(fw_flt, f_intToFloat(n1) < f_intToFloat(n2))
DEF_BIN_ARITH_WORD(fw_fgt, f_intToFloat(n1) > f_intToFloat(n2))
DEF_BIN_ARITH_WORD(fw_flte, f_intToFloat(n1) <= f_intToFloat(n2))
DEF_BIN_ARITH_WORD(fw_fgte, f_intToFloat(n1) >= f_intToFloat(n2))

DEFWORD(fw_print, {
	f_Int n;
	TRY_POP(&n);
	fprintf(stderr, "%lu ", n);
	return true;
})

DEFWORD(fw_fprint, {
	f_Int n;
	TRY_POP(&n);
	fprintf(stderr, "%f ", f_intToFloat(n));
	return true;
})

DEFWORD(fw_fetch, {
	f_Int addr;
	TRY_POP(&addr);
	TRY_PUSH(*(f_Int*)addr);
	return true;
})

DEFWORD(fw_store, {
	f_Int addr;
	f_Int val;
	TRY_POP(&addr);
	TRY_POP(&val);
	*(f_Int*)addr = val;
	return true;
})

bool fw_words(f_State *s) {
	Dict_Iterator it = Dict_iter(&s->words);
	Dict_Entry *e = NULL;
	while ((e = Dict_iterNext(&it)) != NULL) {
		fprintf(stderr, "%s ", e->key);
	}
	return true;
}

bool fw_show(f_State *s) {
	size_t i = 0;
	for (; i < s->working_stack.len; i++) {
		fprintf(stderr, "%lu ", s->working_stack.buf[i]);
	}
	return true;
}

bool fw_quit(f_State *s) {
	s->is_closed = true;
	return true;
}

bool fw_beginWordCompile(f_State *s) {
	ArrayList b;
	SliceConst wordname;
	SliceConst w;

	b = ArrayList_init();

	wordname = f_State_getToken(s);
	if (wordname.ptr == NULL) {
		logD("missing word name");
		goto error;
	}

	while ((w = f_State_getToken(s)).ptr != NULL) {
		if (w.len == 1 && w.ptr[0] == ';') {
			f_Word w;
			f_Word *mem;

			w.tag = F_WORDT_BYTECODE;
			w.is_immediate = false;
			w.un.bytecode = b;

			mem = malloc(sizeof(f_Word));
			if (mem == NULL) die("OOM");
			*mem = w;

			Dict_put(&s->words, wordname, (void*)mem);
			return true;
		}

		f_State_compileSingleWord(s, &b, w);
	}

	logD("MissingToken(\";\")");

error:
	ArrayList_deinit(&b);
	return false;
}

bool fw_beginComment(f_State *s) {
	SliceConst w;
	while ((w = f_State_getToken(s)).ptr != NULL) {
		if (w.len == 1 && w.ptr[0] == ')') {
			return true;
		}
	}

	logD("MissingToken(\")\")");
	return false;
}

bool fw_beginComment2(f_State *s) {
	SliceConst w;
	while ((w = f_State_getToken(s)).ptr != NULL) {
		if (w.len == 2 && w.ptr[0] == ')' && w.ptr[1] == ')') {
			return true;
		}
	}

	logD("missing ))");
	return false;
}

bool fw_defVar(f_State *s) {
	SliceConst name;
	f_Int *cell;
	ArrayList b;
	size_t ptr_int, i;
	f_Word w;
	f_Word *mem;

	name = f_State_getToken(s);
	if (name.ptr == 0) {
		logD("MissingToken(VarName)");
		return false;
	}

	cell = malloc(sizeof(f_Int));
	if (cell == 0) {
		logD("OOM");
		return false;
	}

	b = ArrayList_init();
	ArrayList_push(&b, F_INS_PUSHINT);
	ptr_int = (size_t)cell;
	for (i = sizeof(size_t); i > 0; i--) {
		ArrayList_push(&b, (size_t) (ptr_int >> (i * 8 - 8)));
	}

	w.tag = F_WORDT_BYTECODE;
	w.is_immediate = false;
	w.un.bytecode = b;

	mem = malloc(sizeof(f_Word));
	if (mem == NULL) die("OOM");
	*mem = w;

	Dict_put(&s->words, name, (void*)mem);
	return true;
}

bool fw_beginIf(f_State *s) {
	ArrayList ift = ArrayList_init();

	SliceConst w;
	while ((w = f_State_getToken(s)).ptr != NULL) {
		if (w.len == 1 && w.ptr[0] == ')') {
			size_t len_int, i;
			ArrayList *b = s->bytecode;

			ArrayList_push(b, F_INS_JMPCOND);
			len_int = b->items.len;
			for (i = sizeof(size_t); i > 0; i--) {
				ArrayList_push(b, (size_t) (len_int >> (i * 8 - 8)));
			}
			for (i = 0; i < ift.items.len; i++) {
				ArrayList_push(b, ift.items.ptr[i]);
			}

			ArrayList_deinit(&ift);
			return true;
		}

		f_State_compileSingleWord(s, &ift, w);
	}

	logD("MissingToken(\")\")");
	ArrayList_deinit(&ift);
	return false;
}
