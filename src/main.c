#include "dict.h"
#include "forth.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO: use GNU readline if available */

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

	/* TODO: deinit */

	size_t linelen = 128;
	char *linebuf = malloc(linelen);
	if (linebuf == NULL) printf("OOM\n");
	while (!s.is_closed) {
		fprintf(stderr, "%s", PROMPT_STRING);

		/* FIXME: this will split up around the 127th char so bruh */
		size_t i;
		for (i = 0; i < linelen - 1; i++) {
			int c = fgetc(stdin);
			if (c == '\n') break;
			if (c == EOF) {
				s.is_closed = true;
				break;
			}
			linebuf[i] = (char) c;
		}
		linebuf[i] = '\0';
		f_State_evalString(&s, linebuf, false);
	}

	return 0;
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
	fprintf(stderr, "%d", n);
	return true;
})

bool fw_words(f_State *s) {
	Dict_Iterator it = Dict_iter(&s->words);
	Dict_Entry *e = NULL;
	while ((e = Dict_iterNext(&it)) != NULL) {
		fprintf(stderr, "%s ", e->key);
	}
}

bool fw_show(f_State *s) {
	size_t i = 0;
	for (; i < s->working_stack.len; i++) {
		fprintf(stderr, "%d ", s->working_stack.buf[i]);
	}
}
