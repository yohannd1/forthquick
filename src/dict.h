#ifndef _DICT_H
#define _DICT_H

/* adapted from https://stackoverflow.com/a/4384446 */

#include "ArrayList.h"
#include "bool.h"

#include <stddef.h>

typedef struct Dict_Entry {
	struct Dict_Entry *next;
	char *key;
	void *value;
} Dict_Entry;

typedef struct Dict {
	Dict_Entry **buf;
	size_t hash_len;
} Dict;

typedef struct Dict_Iterator {
	Dict *dict;
	Dict_Entry *current_node;
	size_t current_idx;
} Dict_Iterator;

typedef size_t Dict_Hash;

bool Dict_init(Dict *dict, size_t hash_len);
Dict_Entry *Dict_find(Dict *dict, const char *query);
Dict_Entry *Dict_findN(Dict *dict, SliceConst query);
Dict_Entry *Dict_put(Dict *dict, SliceConst key, void *val);
Dict_Hash Dict_hash(Dict *dict, const char *str);
Dict_Hash Dict_hashN(Dict *dict, SliceConst str);

Dict_Iterator Dict_iter(Dict *dict);
Dict_Entry *Dict_iterNext(Dict_Iterator *iter);

#endif
