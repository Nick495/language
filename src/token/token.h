#ifndef TOKEN_H_
#define TOKEN_H_

#include <assert.h>		/* assert() */
#include <string.h>		/* memcpy(), strlen() */
#include <stddef.h>		/* size_t */
#include "mem/mem.h"	/* mem_alloc(), mem_realloc(), mem_free() */

#define TOKEN_TYPE_COUNT 3
enum token_type {
	TOKEN_START,
	TOKEN_EOF,
	TOKEN_NUMBER,
	TOKEN_OPERATOR,
	TOKEN_LPAREN,
	TOKEN_RPAREN
};

typedef struct token_* token;

/* slen = strlen(s) */
token token_make(enum token_type type, const char* s, size_t slen, token prev);
void token_free(token);
token token_copy(token dst, token src);
char* get_value(token);
enum token_type get_type(token);
size_t token_size();
#endif
