#ifndef _FORTH_H
#define _FORTH_H

#include "dict.h"
#include "string.h"

#include <stdbool.h>
#include <stdint.h>

typedef int64_t f_Item;

typedef struct f_Stack {
	f_Item *buf;
	size_t len;
	size_t capacity;
} f_Stack;

bool f_Stack_init(f_Stack *f);
bool f_Stack_push(f_Stack *f, f_Item i);
bool f_Stack_pop(f_Stack *f, f_Item *dest);

typedef struct f_State {
	Dict words;
	f_Stack working_stack;
	f_Stack return_stack;
	bool is_closed;
	const char *reader_str;
	size_t reader_idx;
	const char *prompt_string;
} f_State;

typedef bool (*f_WordFunc)(f_State *);
typedef struct f_Word {
	f_WordFunc func;
	bool is_immediate;
} f_Word;

bool f_State_init(f_State *dest);
void f_State_defineWord(f_State *s, const char *wordname, f_WordFunc func, bool is_immediate);
void f_State_evalString(f_State *s, const char *line, bool echo);
StringView f_State_getToken(f_State *s);

#endif
