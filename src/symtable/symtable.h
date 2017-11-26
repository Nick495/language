#include "ASTNode/ASTNode.h" /* ASTNode, struct ast_node */
#include "mem/mem.h"	 /* mem_alloc(), mem_realloc(), mem_dealloc() */
#include "xxhash.h"	  /* hash library */
#include <assert.h>	  /* assert() */
#include <stddef.h>	  /* size_t */
#include <string.h>	  /* strlen(), memcmp() */

struct symtable;

struct symtable *symtable_make(struct slab_alloc *a);
void symtable_free(struct symtable *s, struct slab_alloc *a);
size_t symtable_push(struct symtable *s, struct slab_alloc *a, const char *key,
		     struct ast_node n);
ASTNode symtable_find(struct symtable *s, const char *key, size_t len);
