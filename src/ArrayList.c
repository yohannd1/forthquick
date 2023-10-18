#include "ArrayList.h"

#include <stdlib.h>

/* TODO: exponential growth in capacity for less allocations */

#define max(a, b) (a) > (b) ? (a) : (b)

SliceConst SliceConst_fromCString(const char *s) {
	SliceConst r;
	r.ptr = (const uint8_t*)s;
	r.len = strlen(s);
	return r;
}

SliceConst SliceConst_fromMut(SliceMut mut) {
	SliceConst r;
	r.ptr = mut.ptr;
	r.len = mut.len;
	return r;
}

SliceConst SliceConst_newEmpty(void) {
	SliceConst r;
	r.ptr = NULL;
	r.len = 0;
	return r;
}

static bool ArrayList_resize(ArrayList *al);

ArrayList ArrayList_init(void) {
	ArrayList r;
	r.items.ptr = NULL;
	r.items.len = 0;
	r.capacity = 0;
	return r;
}

void ArrayList_deinit(ArrayList *al) {
	free(al->items.ptr);
}

bool ArrayList_reserveNMore(ArrayList *al, size_t amount) {
	size_t current = max(al->items.len, al->capacity);
	return ArrayList_updateCapacity(al, current+amount);
}

bool ArrayList_updateCapacity(ArrayList *al, size_t capacity) {
	if (al->capacity == capacity) return true;

	al->capacity = capacity;
	return ArrayList_resize(al);
}

static bool ArrayList_resize(ArrayList *al) {
	size_t memsize;
	uint8_t *m;

	memsize = max(al->items.len, al->capacity);
	if (memsize == 0) {
		if (al->items.ptr != NULL) {
			free(al->items.ptr);
			al->items.ptr = NULL;
		}
		return true;
	}
	
	m = (al->items.ptr == NULL) ? malloc(memsize) : realloc(al->items.ptr, memsize);
	if (m == NULL) return false;

	al->items.ptr = m;
	return true;
}

bool ArrayList_push(ArrayList *al, uint8_t item) {
	al->items.len += 1;
	if (!ArrayList_resize(al)) return false;
	al->items.ptr[al->items.len - 1] = item;
	return true;
}

bool ArrayList_pop(ArrayList *al, uint8_t *dest) {
	if (al->items.len == 0) return false;
	*dest = al->items.ptr[al->items.len - 1];
	al->items.len -= 1;
	ArrayList_resize(al); /* ignore realloc error */
	return true;
}
