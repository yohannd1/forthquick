#ifndef _ARRAYLIST_H
#define _ARRAYLIST_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "bool.h"

#define SLICE_FROMNUL(s) (SliceConst){ .ptr = (const uint8_t*)s, .len = strlen(s) }
#define SLICE_MUT2CONST(mut) (SliceConst){ .ptr = mut.ptr, .len = mut.len }

typedef struct SliceMut {
	uint8_t *ptr;
	size_t len;
} SliceMut;

typedef struct SliceConst {
	const uint8_t *ptr;
	size_t len;
} SliceConst;

typedef struct ArrayList {
	SliceMut items;
	size_t capacity;
} ArrayList;

ArrayList ArrayList_init(void);
void ArrayList_deinit(ArrayList *al);
bool ArrayList_reserveNMore(ArrayList *al, size_t amount);
bool ArrayList_updateCapacity(ArrayList *al, size_t capacity);
bool ArrayList_push(ArrayList *al, uint8_t item);
bool ArrayList_pop(ArrayList *al, uint8_t *dest);

#endif
