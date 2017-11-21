#ifndef MEM_H_
#define MEM_H_

#include <assert.h> /* assert() */
#include <stddef.h> /* size_t */
#include <stdint.h> /* uint32_t */
#include <stdlib.h> /* malloc(), realloc(), free() */
#include <string.h> /* memcpy() */

struct slab_alloc {       /* 32 bytes */
	size_t slab_size; /* 8 Slab size. */
	void *cur;	/* 16 First free address of current slab. */
	void **next;      /* 24 next page address */
	void *head;       /* 32 Head of allocated slab list. */
};
/*
 * Requires that elements are less than 2^32 bytes in size.
 * Requires that there are less than 2^32 elements per slab.
 * Minimum element size is the size of a pointer.
*/
struct pool_alloc {		 /* 36 bytes */
	void *used;		 /* 8 alloced ptr within slab */
	void *base;		 /* 16 Base pointer into page. */
	void *free;		 /* 24 free list */
	struct slab_alloc *slab; /* 32 slab alloc */
	uint32_t element_size;   /* 36 element size */
};

/* Slab allocator. */
int init_slab_alloc(struct slab_alloc *a, size_t slab_size);
void *mem_slab_alloc(struct slab_alloc *a, size_t size);
void *mem_slab_realloc(struct slab_alloc *a, size_t new_size, void *ptr,
		       size_t prev_size);
void mem_slab_free(struct slab_alloc *a, void *ptr);
void mem_slab_deinit(struct slab_alloc *a);

/* Pool allocator. */
int init_pool_alloc(struct pool_alloc *a, struct slab_alloc *s, uint32_t esz);
void *mem_pool_alloc(struct pool_alloc *a);
void mem_pool_free(struct pool_alloc *a, void *ptr);
void mem_pool_deinit(struct pool_alloc *a);

#endif
