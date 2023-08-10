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
bool fw_printInt(f_State *s);
bool fw_words(f_State *s);
bool fw_show(f_State *s);
bool fw_quit(f_State *s);
bool fw_beginWordCompile(f_State *s);
bool fw_beginComment(f_State *s);
bool fw_beginComment2(f_State *s);

int main(void) {
	f_State s;
	if (!f_State_init(&s)) die("failed to init state");

	f_State_defineWord(&s, "+", fw_add, false);
	f_State_defineWord(&s, "-", fw_sub, false);
	f_State_defineWord(&s, "*", fw_mul, false);
	f_State_defineWord(&s, "/", fw_div, false);
	f_State_defineWord(&s, ".", fw_printInt, false);
	f_State_defineWord(&s, "words", fw_words, false);
	f_State_defineWord(&s, "show", fw_show, false);
	f_State_defineWord(&s, "quit", fw_quit, false);
	f_State_defineWord(&s, ":", fw_beginWordCompile, true);
	f_State_defineWord(&s, "(", fw_beginComment, true);
	f_State_defineWord(&s, "((", fw_beginComment2, true);

	s.echo = true;
	f_State_compileAndRun(&s, SLICE_FROMNUL("( Welcome to QuickForth v0.3. )"));
	f_State_compileAndRun(&s, SLICE_FROMNUL("( Type 'words' to list words. )"));
	s.echo = false;

	size_t linelen = 128;
	char *linebuf = malloc(linelen);
	if (linebuf == NULL) printf("OOM\n");
	while (!s.is_closed) {
		char *line = promptReadLine(&s);
		if (line == NULL) {
			s.is_closed = true;
			goto read_end;
		}

		f_State_compileAndRun(&s, SLICE_FROMNUL(line));
		fprintf(stderr, "ok\n");
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
		logE("OOM");
		return NULL;
	}

	size_t i = 0;
	for (;; i++) {
		if (i >= bufsize) {
			bufsize += coarse_amount;
			char *new = realloc(mem, bufsize);
			if (new == NULL) {
				logE("OOM");
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

#define TRY_POP(var)                                 \
	f_Int var;                                  \
	if (!f_Stack_pop(&s->working_stack, &var)) { \
		LOG("StackUnderflow ");              \
		return false;                        \
	}

#define TRY_PUSH(val)                                \
	if (!f_Stack_push(&s->working_stack, val)) { \
		LOG("OOM ");         \
		return false;                        \
	}

#define DEFWORD(name, statements) bool name(f_State *s) statements

DEFWORD(fw_add, {
	TRY_POP(n2);
	TRY_POP(n1);
	TRY_PUSH(n1 + n2);
	return true;
})

DEFWORD(fw_sub, {
	TRY_POP(n2);
	TRY_POP(n1);
	TRY_PUSH(n1 - n2);
	return true;
})

DEFWORD(fw_mul, {
	TRY_POP(n2);
	TRY_POP(n1);
	TRY_PUSH(n1 * n2);
	return true;
})

DEFWORD(fw_div, {
	TRY_POP(n2);
	TRY_POP(n1);
	TRY_PUSH(n1 / n2);
	return true;
})

DEFWORD(fw_printInt, {
	TRY_POP(n);
	fprintf(stderr, "%d ", n);
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
		fprintf(stderr, "%d ", s->working_stack.buf[i]);
	}
	return true;
}

bool fw_quit(f_State *s) {
	s->is_closed = true;
	return true;
}

bool fw_beginWordCompile(f_State *s) {
	ArrayList b = ArrayList_init();

	SliceConst defword = f_State_getToken(s);
	if (defword.ptr == NULL) {
		logD("missing word to define");
		goto error;
	}

	SliceConst w;
	while ((w = f_State_getToken(s)).ptr != NULL) {
		if (w.len == 1 && w.ptr[0] == ';') {
			f_Word w;
			w.tag = F_WORDT_BYTECODE;
			w.is_immediate = false;
			w.un.bytecode = b;

			f_Word *mem = malloc(sizeof(f_Word));
			if (mem == NULL) die("OOM");
			*mem = w;

			Dict_put(&s->words, defword, (void*)mem);
			return true;
		}

		f_State_compileSingleWord(s, &b, w);
	}

	logD("missing ;");

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

	logD("missing )");
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
