#ifndef _ARRAYLIST_H
#define _ARRAYLIST_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

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

SliceConst SliceConst_fromCString(const char *s);
SliceConst SliceConst_fromMut(SliceMut mut);
SliceConst SliceConst_newEmpty(void);

ArrayList ArrayList_init(void);
void ArrayList_deinit(ArrayList *al);
bool ArrayList_reserveNMore(ArrayList *al, size_t amount);
bool ArrayList_updateCapacity(ArrayList *al, size_t capacity);
bool ArrayList_push(ArrayList *al, uint8_t item);
bool ArrayList_pop(ArrayList *al, uint8_t *dest);

#endif
