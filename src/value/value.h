#ifndef VALUE_H_
#define VALUE_H_

#include "mem/mem.h" /* Memory management. */
#include <assert.h>  /* assert() */
#include <stdio.h>   /* snprintf() */
#include <stdlib.h>  /* malloc(), realloc(), free() */
#include <string.h>  /* memcpy() */

typedef struct Value_ *Value;

enum value_type { INTEGER, VECTOR, SYMBOL, FUNCTION, ERROR };
enum error_type { INVALID_ARGS, MEM_ALLOC };
union value_data {
	size_t shape;
	unsigned long integer;
	Value vector;
	size_t symbol;
	struct function_obj {
		size_t func_rank; /* TODO: What should this do? */
		enum arity { ZERO, ONE, TWO } arity;
		size_t hash;
	} function;
	enum error_type error;
};

struct value_atom {
	enum value_type type;
	union value_data data;
};

struct value_vm {
	struct pool_alloc pool;
	struct slab_alloc slab;
};
typedef struct value_vm *Vm;

Vm value_make_vm(void);
void value_free_vm(Vm vm);

Value value_make(Vm vm, struct value_atom value, size_t isize);
Value value_append(Vm vm, Value v, struct value_atom val);

Value value_add(Vm vm, Value a, Value w);
Value value_reference(Value v);
void value_free(Vm vm, Value v);
void value_free_vm(Vm vm);
char *value_stringify(Value v);
#endif
