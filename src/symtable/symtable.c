#include "symtable/symtable.h"

struct entry {
	const char *key;
	void *value;
	/* 0 so it can be set with memset. */
	enum { EMPTY = 0, DELETED, SET } status;
};
struct symtable {
	size_t use;
	size_t cap;
	size_t seed;
	size_t esize;	 /* Size of an element */
	struct slab_alloc *a; /* Slab allocator to use */
	struct entry *entries;
};

static size_t alloc_size(struct symtable *s)
{
	assert(s);
	return sizeof(*s->entries) + s->esize;
}

static void init_entries(struct entry *es, size_t cap)
{
	size_t i;
	for (i = 0; i < cap; ++i) {
		es[i].status = EMPTY;
		es[i].key = NULL;
		es[i].value = &es[i] + sizeof(*es);
	}
}

#define CAP 100
struct symtable *symtable_make(struct slab_alloc *a, size_t esize)
{
	struct symtable *s;
	s = mem_slab_alloc(a, sizeof *s + (sizeof(*s->entries) + esize) * CAP);
	assert(s); /* TODO: Error handling. */
	s->use = 0;
	s->cap = CAP;
	s->seed = 0;
	s->a = a;
	s->esize = esize;
	s->entries = (struct entry *)((char *)s + sizeof(*s));
	init_entries(s->entries, CAP);
	return s;
}

static struct entry *next(struct symtable *s, size_t *hash)
{
	assert(s);
	assert(hash);
	*hash = *hash + 1; /* Linear probing. */

	return &s->entries[*hash % s->cap];
}

/* Inserts an entry with linear probing. */
static size_t insert_entry(struct symtable *s, const char *key, void *v)
{
	size_t hash = XXH64(key, strlen(key), s->seed);
	struct entry *e = &(s->entries[hash % s->cap]);
	while (e->status == SET && strcmp(e->key, key)) {
		e = next(s, &hash);
	}
	e->status = SET;
	e->key = key;
	memcpy(e->value, v, s->esize);
	return hash;
}

/* Double symtable's size. */
static void symtable_expand(struct symtable *s)
{
	struct entry *old_entries = s->entries;
	size_t i, old_cap = s->cap;
	s->entries = mem_slab_alloc(s->a, alloc_size(s) * s->cap * 2);
	assert(s->entries); /* TODO: Error handling. */
	s->cap *= 2;
	init_entries(s->entries, s->cap);
	for (i = 0; i < old_cap; ++i) {
		if (old_entries[i].status != SET) {
			continue;
		}
		insert_entry(s, old_entries[i].key, old_entries[i].value);
	}
	mem_slab_free(s->a, old_entries);
}

size_t symtable_push(struct symtable *s, const char *key, void *value)
{
	if (s->use >= (s->cap * 3) / 4) {
		symtable_expand(s);
	}
	return insert_entry(s, key, value);
}

void *symtable_find(struct symtable *s, const char *key, size_t len)
{
	size_t hash = XXH64(key, len, s->seed);
	struct entry *e = &s->entries[hash % s->cap];
	while (e->status == DELETED ||
	       (e->status == SET && memcmp(e->key, key, len))) {
		e = next(s, &hash);
	}
	if (e->status == SET) {
		return e->value;
	} else {
		return NULL;
	}
}

void symtable_free(struct symtable *s) { mem_slab_free(s->a, s); }
