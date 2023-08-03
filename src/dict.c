#include "dict.h"
#include "string.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

bool stringEqN0(StringView s1, const char *s2);
bool stringEq00(const char *s1, const char *s2);
char *strDup(const char *str);

bool Dict_init(Dict *dict, size_t hash_len) {
	Dict_Entry **buf = malloc(sizeof(Dict_Entry *) * hash_len);
	if (buf == NULL) return false;

	size_t i;
	for (i = 0; i < hash_len; i++)
		buf[i] = NULL;

	dict->buf = buf;
	dict->hash_len = hash_len;

	return true;
}

Dict_Entry *Dict_find(Dict *dict, const char *query) {
	Dict_Entry *ep = dict->buf[Dict_hash(dict, query)];
	for (; ep != NULL; ep = ep->next)
		if (stringEq00(query, ep->key) == 0) return ep;
	return NULL;
}

Dict_Entry *Dict_findN(Dict *dict, StringView query) {
	Dict_Entry *ep = dict->buf[Dict_hashN(dict, query)];
	for (; ep != NULL; ep = ep->next)
		if (stringEqN0(query, ep->key)) return ep;
	return NULL;
}

Dict_Entry *Dict_put(Dict *dict, const char *key, void *val) {
	Dict_Entry *ep = Dict_find(dict, key);
	if (ep == NULL) {
		ep = malloc(sizeof(*ep));
		if (ep == NULL) return NULL;

		ep->key = strDup(key);
		if (ep->key == NULL) return NULL;

		size_t hashval = Dict_hash(dict, key);
		ep->next = dict->buf[hashval];
		dict->buf[hashval] = ep;
	} else {
		free(ep->value); /* free previous entry */
	}

	ep->value = val;
	return ep;
}

Dict_Hash Dict_hash(Dict *dict, const char *str) {
	size_t h = 0;
	for (; *str != '\0'; str++)
		h = *str + 31 * h;
	return h % dict->hash_len;
}

Dict_Hash Dict_hashN(Dict *dict, StringView str) {
	size_t i = 0;
	size_t h = 0;
	for (; i < str.len; i++)
		h = str.ptr[i] + 31 * h;
	return h % dict->hash_len;
}

Dict_Iterator Dict_iter(Dict *dict) {
	Dict_Iterator r;
	r.dict = dict;
	r.current_node = NULL;
	r.current_idx = 0;
	return r;
}

Dict_Entry *Dict_iterNext(Dict_Iterator *it) {
	while (true) {
		Dict_Entry *cn = it->current_node;
		if (cn == NULL) {
			if (it->current_idx == it->dict->hash_len - 1) return NULL;
			it->current_idx++;
			it->current_node = it->dict->buf[it->current_idx];
			continue;
		}

		it->current_node = it->current_node->next;
		return cn;
	}
}

bool stringEqN0(StringView s1, const char *s2) {
	size_t i = 0;
	while (true) {
		if (i >= s1.len && s2[i] == '\0') return true; /* both ended */
		if (i >= s1.len || s2[i] == '\0') return false; /* only one ended */
		if (s1.ptr[i] != s2[i]) return false; /* different chars */
		i++;
	}
}

bool stringEq00(const char *s1, const char *s2) {
	size_t i = 0;
	while (true) {
		if (s1[i] != s2[i]) return false;
		if (s1[i] == '\0') return true;
		i++;
	}
}

char *strDup(const char *str) {
	size_t len = 0;
	for (; str[len] != '\0'; len++)
		;

	char *p = malloc(len + 1); /* +1 for ’\0’ */
	if (p == NULL) return NULL;
	size_t i = 0;
	for (; i < len + 1; i++)
		p[i] = str[i];
	return p;
}
