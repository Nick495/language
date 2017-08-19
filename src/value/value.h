#ifndef VALUE_H_
#define VALUE_H_

#include <assert.h>		/* assert() */
#include <stdio.h>		/* snprintf() */
#include <string.h>		/* memcpy() */
#include "mem/mem.h"	/* mem_alloc(), mem_free() */

typedef struct Value_* Value;

enum value_type { VALUE_NUMBER, VALUE_VECTOR };
Value value_make_number(size_t value);
Value value_make_vector(unsigned long value);
Value value_append(Value v, unsigned long val);
Value value_add(Value a, Value w);
Value value_reference(Value v);
void value_free(Value v);
char* value_stringify(Value v);
#endif
