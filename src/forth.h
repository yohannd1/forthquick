#ifndef _FORTH_H
#define _FORTH_H

#include "bool.h"
#include "dict.h"
#include "ArrayList.h"

#include <stdint.h>

typedef size_t f_Int; /* TODO: perhaps call this cell? since i'm even using this to represent floats... */
/* FIXME: assert sizeof(f_Int) >= sizeof(float) */

typedef struct f_Stack {
	f_Int *buf;
	size_t len;
	size_t capacity;
} f_Stack;

bool f_Stack_init(f_Stack *f);
bool f_Stack_push(f_Stack *f, f_Int i);
bool f_Stack_pop(f_Stack *f, f_Int *dest);

float f_intToFloat(f_Int i);
f_Int f_floatToInt(float f);

typedef struct f_State {
	Dict words;
	ArrayList *bytecode; /* TODO: rename this? it's the compilation bytecode and rn it's very abiguous */
	f_Stack working_stack;
	f_Stack return_stack;
	const uint8_t *next_byte;
	bool is_closed;
	SliceConst reader_str;
	size_t reader_idx;
	const char *prompt_string;
	bool echo;
} f_State;

typedef bool (*f_WordFunc)(f_State *);
typedef struct f_Word {
	uint8_t tag;
	bool is_immediate;
	union {
		ArrayList bytecode;
		f_WordFunc func_ptr;
	} un;
} f_Word;

typedef enum f_WordType {
	F_WORDT_FUNC = 0,
	F_WORDT_BYTECODE
} f_WordType;

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
bool f_State_compileSingleWord(f_State *s, ArrayList *byte_al, SliceConst word);

typedef enum f_Instruction {
	F_INS_RETURN = 0, /* return */
	F_INS_PUSHINT, /* push an int to the stack */
	F_INS_JMPWORD, /* jump to a word */
	F_INS_CALLWORD, /* call a word (jump + push to return stack) */
	F_INS_PWRITE, /* TODO: what is this again? */
	F_INS_PREAD, /* TODO: what is this again? */
	F_INS_JMPCOND
} f_Instruction;

#endif
