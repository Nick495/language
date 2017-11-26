#include "mem.h"

#include <stdio.h>
static int add_slab(struct slab_alloc *a)
{
	assert(a);
	assert(*(a->next) == NULL);
#if MEM_PERF
	printf("Malloc()ing slab\n");
#endif
	void *next = malloc(a->slab_size + sizeof(*a->next));
	if (!next) {
		return 1;
	}
	/* Encode next pointer in bottom bytes of this page. */
	*(a->next) = next;
	/* Update the next pointer to the bottom of the page, and offset cur. */
	a->next = next;
	*(a->next) = NULL;
	a->cur = (void *)((char *)next + sizeof(*a->next));
	return 0;
}

int init_slab_alloc(struct slab_alloc *a, size_t slab_size)
{
	assert(a);
	a->slab_size = slab_size;
	a->next = a->head = malloc(a->slab_size);
	if (!a->next) {
		return 1;
	}
	*(a->next) = NULL; /* Init the end of the free pointer list. */
	a->cur = (char *)a->next + sizeof(*a->next);
	assert(*(a->next) == NULL);
	return 0;
}

static size_t max_slab_alloc_sz(struct slab_alloc *a)
{
	return a->slab_size - sizeof(*a->next);
}

void *mem_slab_alloc(struct slab_alloc *a, size_t size)
{
	void *tmp;
	assert(a);
	if (size > max_slab_alloc_sz(a)) {
/* We're never going to be able to slab this */
#ifdef MEM_PERF
		printf("Malloc()ing big alloc\n");
#endif
		return malloc(size);
	}
	if (size > a->slab_size - ((char *)a->cur - (char *)a->next)) {
		/* a->next is sitting at the base of the page, so we're good. */
		if (add_slab(a)) {
			return NULL;
		}
		assert(((char *)a->cur - (char *)a->next) == sizeof(*a->next));
		return mem_slab_alloc(a, size);
	}
	tmp = a->cur;
	a->cur = (char *)tmp + size;
	return tmp;
}

void mem_slab_deinit(struct slab_alloc *a)
{
	void *next, *tmp = NULL;
	assert(a);
	for (next = a->head; next; next = *((void **)next)) {
		free(tmp);
		tmp = next;
	}
}

void *mem_slab_realloc(struct slab_alloc *a, size_t new_size, void *ptr,
		       size_t prev_size)
{
	void *new;
	/* Fast path, unlikely. */
	if (((ptr > (void *)a->next) && (ptr <= a->cur)) && /* Current page */
	    new_size < a->slab_size - sizeof(*(a->next))) { /* Got space */
		/* Just bump the pointer. */
		a->cur = (void *)((char *)ptr + new_size);
		return ptr;
	}
	new = mem_slab_alloc(a, new_size);
	if (!new) {
		return NULL;
	}
	memcpy(new, ptr, prev_size);
	return new;
}

/* Whee! */
void mem_slab_free(struct slab_alloc *a, void *ptr)
{
	(void)a;
	(void)ptr;
}

static void assert_valid_pool_alloc(struct pool_alloc *a)
{
	assert(a);
	assert(a->slab);
	assert(a->used != NULL);
	assert(a->element_size > sizeof(a->free));
}

static int add_pool(struct pool_alloc *a)
{
	a->base = a->used = mem_slab_alloc(a->slab, max_slab_alloc_sz(a->slab));
	if (!a->used) {
		return 1;
	}
	return 0;
}

int init_pool_alloc(struct pool_alloc *a, struct slab_alloc *s, uint32_t esize)
{
	assert(a);
	assert(s);
	a->free = NULL;
	a->slab = s;
	a->element_size = esize > sizeof(a->free) ? esize : sizeof(a->free);
	if (add_pool(a)) {
		return 1;
	}
	assert_valid_pool_alloc(a);
	return 0;
}

void *mem_pool_alloc(struct pool_alloc *a)
{
	void *tmp;
	assert_valid_pool_alloc(a);
	if (a->free) {
		tmp = a->free;
		a->free = *((void **)a->free);
	}

	if (a->element_size >
	    a->slab->slab_size - ((char *)a->used - (char *)a->base)) {
		tmp = a->used;
		add_pool(a);
		if (!a->used) {
			a->used = tmp;
			return NULL;
		}
	}
	tmp = a->used;
	a->used = (void *)((char *)a->used + a->element_size);

	assert_valid_pool_alloc(a);
	return tmp;
}

void mem_pool_free(struct pool_alloc *a, void *ptr)
{
	*((void **)(ptr)) = a->free;
	a->free = ptr;
	return;
}

/* Nothing to do here, the slab has to free everything. */
void mem_pool_deinit(struct pool_alloc *a) { (void)a; }
