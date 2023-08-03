#include "dict.h"
#include "forth.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAS_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define logD(msg) /* TODO */
#define logW(msg) /* TODO */
#define logE(msg) /* TODO */

char *promptReadLine(f_State *s);

#define die(msg)                                     \
	{                                            \
		fprintf(stderr, "error: %s\n", msg); \
		exit(1);                             \
	}

bool fw_add(f_State *s);
bool fw_sub(f_State *s);
bool fw_mul(f_State *s);
bool fw_div(f_State *s);
bool fw_printInt(f_State *s);
bool fw_words(f_State *s);
bool fw_show(f_State *s);

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

	f_State_evalString(&s, "\\ Welcome to QuickForth v0.3.", true);
	f_State_evalString(&s, "\\ Type 'words' to list words.", true);
	f_State_evalString(&s, "", true);

	size_t linelen = 128;
	char *linebuf = malloc(linelen);
	if (linebuf == NULL) printf("OOM\n");
	while (!s.is_closed) {
		char *line = promptReadLine(&s);
		if (line == NULL) {
			s.is_closed = true;
			goto read_end;
		}

		f_State_evalString(&s, line, false);
read_end:
		free(line);
	}

	/* TODO: deinit */

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
	f_Item var;                                  \
	if (!f_Stack_pop(&s->working_stack, &var)) { \
		LOG("stack underflow");              \
		return false;                        \
	}

#define TRY_PUSH(val)                                \
	if (!f_Stack_push(&s->working_stack, val)) { \
		LOG("stack overflow / OOM");         \
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
	fprintf(stderr, "%ld", n);
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
		fprintf(stderr, "%ld ", s->working_stack.buf[i]);
	}
	return true;
}
