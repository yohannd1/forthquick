#ifndef _FORTH_H
#define _FORTH_H

#include "dict.h"
#include "ArrayList.h"

#include <stdbool.h>
#include <stdint.h>

typedef int32_t f_Int;

typedef struct f_Stack {
	f_Int *buf;
	size_t len;
	size_t capacity;
} f_Stack;

bool f_Stack_init(f_Stack *f);
bool f_Stack_push(f_Stack *f, f_Int i);
bool f_Stack_pop(f_Stack *f, f_Int *dest);

typedef struct f_State {
	Dict words;
	f_Stack working_stack;
	f_Stack return_stack;
	bool is_closed;
	SliceConst reader_str;
	size_t reader_idx;
	const char *prompt_string;
	bool echo;
} f_State;

typedef bool (*f_WordFunc)(f_State *);
typedef struct f_Word {
	f_WordFunc func;
	bool is_immediate;
} f_Word;

bool f_State_init(f_State *dest);
/* TODO: void f_State_deinit(f_State *s); */
void f_State_defineWord(f_State *s, const char *wordname, f_WordFunc func, bool is_immediate);

/**
 * Parse and compile `line`, returning the byte
 * The scope of said bytecode is only for this runtime due to pointers.
 *
 * On success: returns true, sets `dest` = the result.
 * On failure: returns false.
 */
bool f_State_compile(f_State *s, SliceConst line, SliceMut *dest);

bool f_State_run(f_State *s, SliceConst bytecode);
void f_State_compileAndRun(f_State *s, SliceConst line);
SliceConst f_State_getToken(f_State *s);

typedef enum f_Instruction {
	F_INS_PUSHINT = 0,
	F_INS_CALLWORD,
} f_Instruction;

#endif
