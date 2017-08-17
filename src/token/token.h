#ifndef TOKEN_H_
#define TOKEN_H_

#include <assert.h>    /* assert() */
#include <string.h>    /* memcpy() */
#include <sys/types.h> /* read() */
#include <sys/uio.h>   /* read() */
#include <unistd.h>    /* read(), write() */
#include "mem/mem.h" /* mem_alloc(), mem_realloc(), mem_free() */

#define TOKEN_TYPE_COUNT 3
enum token_type {
	TOKEN_START,
	TOKEN_EOF,
	TOKEN_NUMBER,
	TOKEN_OPERATOR,
	TOKEN_LPAREN,
	TOKEN_RPAREN
};

typedef struct token_ *token;

/* slen = strlen(s) */
token make_token(enum token_type type, char* str, size_t strlen, token prev);
void free_token(token);
char* get_value(token);
enum token_type get_type(token);
#endif
