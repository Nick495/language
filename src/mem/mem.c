#include "mem.h"

void* mem_alloc(size_t size)
{
	return malloc(size);
}
void* mem_realloc(void* ptr, size_t size)
{
	return realloc(ptr, size);
}
void mem_dealloc(void* ptr)
{
	free(ptr);
}
