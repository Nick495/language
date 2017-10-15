#include <stddef.h>	/* size_t */

#include "value/value.h" /* Value */
#include "mem/mem.h" /* mem_alloc(), mem_realloc(), mem_dealloc() */
#include "xxhash.h" /* hash library */

struct symtable;

struct symtable* symtable_make();
void symtable_free(struct symtable* s);
size_t symtable_push(struct symtable* s, const char* key, Value v);
Value symtable_find(struct symtable* s, const char* key);
