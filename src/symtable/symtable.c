#include "symtable/symtable.h"

struct entry {
	char* key;
	Value value;
	/* 0 so it can be set with memset. */
	enum { EMPTY = 0, DELETED, SET } status;
};
struct symtable {
	size_t use;
	size_t cap;
	size_t seed;
	struct entry* entries;
};

#define CAP 1024
struct symtable* symtable_make()
{
	struct symtable *s;
	s = mem_alloc(sizeof *s + sizeof *s->entries * CAP);
	assert(s); /* TODO: Error handling. */
	s->use = 0;
	s->cap = 1024;
	s->seed = 0;
	return s;
}

static struct entry next(struct symtable* s, size_t* hash)
{
	*hash = *hash + 1; /* Linear probing. */
	return s->entries[*hash % s->cap];
}

/* Inserts an entry with linear probing. */
static size_t insert_entry(struct symtable* s, const char* key, Value v)
{
	size_t hash = XXH64(key, strlen(key), s->seed);
	struct entry e = s->entries[hash % s->cap];
	while (e.status != EMPTY) {
		e = next(s, &hash);
	}
	e.key = (char *) key;
	e.value = v;
	return hash;
}

/* Double symtable's size. */
static void symtable_expand(struct symtable* s)
{
	struct entry* old_entries = s->entries;
	size_t i, old_cap = s->cap;
	s->entries = mem_alloc(sizeof *s->entries * (s->cap * 2));
	assert(s->entries); /* TODO: Error handling. */
	s->cap *= 2;
	memset(s->entries, 0, sizeof *s->entries * s->cap); /* Empty by default */
	for (i = 0; i < old_cap; ++i) {
		if (old_entries[i].status == SET) {
			insert_entry(s, old_entries[i].key, old_entries[i].value);
		}
	}
	mem_dealloc(old_entries);
}

size_t symtable_push(struct symtable* s, const char* key, Value v)
{
	if (s->use >= (s->cap * 3) / 4) {
		symtable_expand(s);
	}
	return insert_entry(s, key, v);
}

Value symtable_find(struct symtable* s, const char* key)
{
	size_t hash = XXH64(key, strlen(key), s->seed);
	struct entry e = s->entries[hash % s->cap];
	while (e.status == SET && memcmp(e.key, key, strlen(key))) {
		e = next(s, &hash);
	}
	if (e.status == SET) {
		return e.value;
	} else {
		return NULL;
	}
}

void symtable_free(struct symtable* s)
{
	if (s) {
		mem_dealloc(s);
	}
}
