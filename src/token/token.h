#ifndef TOKEN_H_
#define TOKEN_H_

#include "mem/mem.h" /* mem_alloc(), mem_realloc(), mem_free() */
#include <assert.h>  /* assert() */
#include <stddef.h>  /* size_t */
#include <string.h>  /* memcpy(), strlen() */

enum token_type {
	TOKEN_START = 0,
	TOKEN_EOF,
	TOKEN_NUMBER,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_SEMICOLON,
	TOKEN_OPERATOR,
	TOKEN_IDENTIFIER,
	TOKEN_LET,
	TOKEN_ASSIGNMENT,
	TOKEN_TYPE_COUNT /* NOT A REAL TOKEN TYPE, merely a count of them. */
};

typedef struct token_ *token;

token token_make(enum token_type type, const char *src, size_t len, size_t off,
		 token prev);
void token_free(token);
token token_copy(token dst, token src);
const char *get_value(token t);
void token_print(token t, char *out);
enum token_type get_type(token);
size_t token_size();
#endif
