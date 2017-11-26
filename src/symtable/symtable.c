#include "symtable/symtable.h"

struct entry {
	const char *key;
	struct ast_node value;
	/* 0 so it can be set with memset. */
	enum { EMPTY = 0, DELETED, SET } status;
};
struct symtable {
	size_t use;
	size_t cap;
	size_t seed;
	struct entry *entries;
};

#define CAP 100
struct symtable *symtable_make(struct slab_alloc *a)
{
	struct symtable *s;
	s = mem_slab_alloc(a, sizeof *s + sizeof *s->entries * CAP);
	assert(s); /* TODO: Error handling. */
	s->use = 0;
	s->cap = CAP;
	s->seed = 0;
	s->entries = (struct entry *)((char *)s + sizeof(*s));
	memset(s->entries, 0, sizeof *s->entries * s->cap);
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
static size_t insert_entry(struct symtable *s, const char *key,
			   struct ast_node n)
{
	size_t hash = XXH64(key, strlen(key), s->seed);
	struct entry *e = &(s->entries[hash % s->cap]);
	while (e->status == SET && strcmp(e->key, key)) {
		e = next(s, &hash);
	}
	e->key = key;
	e->value = n;
	e->status = SET;
	return hash;
}

/* Double symtable's size. */
static void symtable_expand(struct symtable *s, struct slab_alloc *a)
{
	struct entry *old_entries = s->entries;
	size_t i, old_cap = s->cap;
	s->entries = mem_slab_alloc(a, sizeof *s->entries * (s->cap * 2));
	assert(s->entries); /* TODO: Error handling. */
	s->cap *= 2;
	memset(s->entries, 0, sizeof *s->entries * s->cap);
	for (i = 0; i < old_cap; ++i) {
		if (old_entries[i].status != SET) {
			continue;
		}
		insert_entry(s, old_entries[i].key, old_entries[i].value);
	}
	mem_slab_free(a, old_entries);
}

size_t symtable_push(struct symtable *s, struct slab_alloc *a, const char *key,
		     struct ast_node n)
{
	if (s->use >= (s->cap * 3) / 4) {
		symtable_expand(s, a);
	}
	return insert_entry(s, key, n);
}

ASTNode symtable_find(struct symtable *s, const char *key, size_t len)
{
	size_t hash = XXH64(key, len, s->seed);
	struct entry *e = &s->entries[hash % s->cap];
	while (e->status == DELETED ||
	       (e->status == SET && memcmp(e->key, key, len))) {
		e = next(s, &hash);
	}
	if (e->status == SET) {
		return &e->value;
	} else {
		return NULL;
	}
}

void symtable_free(struct symtable *s, struct slab_alloc *a)
{
	mem_slab_free(a, s);
}
