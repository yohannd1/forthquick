#include "ArrayList.h"

#include <stdlib.h>

#define max(a, b) (a) > (b) ? (a) : (b)
static bool ArrayList_resize(ArrayList *al);

ArrayList ArrayList_init() {
	return (ArrayList){
		.items = (SliceMut){ .ptr = NULL, .len = 0 },
		.capacity = 0,
	};
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
	size_t memsize = max(al->items.len, al->capacity);
	if (memsize == 0) {
		if (al->items.ptr != NULL) {
			free(al->items.ptr);
			al->items.ptr = NULL;
		}
		return true;
	}
	
	uint8_t *m = (al->items.ptr == NULL) ? malloc(memsize) : realloc(al->items.ptr, memsize);
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
