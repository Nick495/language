#ifndef VALUE_H_
#define VALUE_H_

#include <assert.h> /* assert() */
#include <stdio.h>	/* snprintf() */
#include <stdlib.h>	/* malloc(), free() */
#include <string.h> /* memcpy() */

typedef struct Value_* Value;

enum value_type { VALUE_NUMBER, VALUE_VECTOR };
Value value_make_number(size_t value);
Value value_make_vec(long *arr, size_t elecount, size_t rank, long *shape);
Value add_values(Value a, Value w);
void value_free(Value v);
char *value_stringify(Value v);
#endif
