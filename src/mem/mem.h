#include <stdlib.h> /* malloc(), realloc(), free() */
#include <stddef.h> /* size_t */

void* mem_alloc(size_t size);
void* mem_realloc(void* ptr, size_t size);
void mem_dealloc(void* ptr);
