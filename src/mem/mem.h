#include <stddef.h> /* size_t */
#include <stdlib.h> /* malloc(), realloc(), free() */

void *mem_alloc(size_t size);
void *mem_realloc(void *ptr, size_t size);
void mem_dealloc(void *ptr);
