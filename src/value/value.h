#ifndef VALUE_H_
#define VALUE_H_

#include "mem/mem.h" /* mem_alloc(), mem_free() */
#include <assert.h>  /* assert() */
#include <stdio.h>   /* snprintf() */
#include <string.h>  /* memcpy() */

typedef struct Value_ *Value;

enum type { INTEGER, VECTOR, SYMBOL, FUNCTION } type;
enum error_type { INVALID_ARGS };
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
	enum type type;
	union value_data data;
};

Value value_make_single(struct value_atom value);
Value value_make_vector(struct value_atom value);
Value value_append(Value v, struct value_atom val);

Value value_add(Value a, Value w);
Value value_reference(Value v);
void value_free(Value v);
char *value_stringify(Value v);
#endif
