#include "token.h"

static void assert_valid_token(token t) { assert(t); }

size_t token_size()
{
	token t;
	return sizeof *t;
}

void token_free(token t) { free(t); }

static token alloc_token(token prev)
{
	token t;
	if (!prev) {
		prev = malloc(sizeof *t);
		if (!prev)
			goto fail_malloc_token;
	}
	t = prev;
	assert_valid_token(t);
	return t;

	token_free(t);
fail_malloc_token:
	return NULL;
}

token token_make(enum token_type type, const char *src, size_t len, size_t off,
		 token prev)
{
	token t = alloc_token(prev);
	if (!t || !src)
		goto fail_alloc_token;
	t->type = type;
	t->len = len;
	t->off = off;
	t->src = src;
	assert_valid_token(t);
	return t;

	token_free(t);
fail_alloc_token:
	return NULL;
}

token token_copy(token dst, token src)
{
	assert_valid_token(src);
	assert_valid_token(dst);
	memcpy(dst, src, sizeof *src); /* Copy internals by value. */
	return dst;
}

enum token_type get_type(token t)
{
	assert_valid_token(t);
	return t->type;
}

size_t get_len(token t)
{
	assert_valid_token(t);
	return t->len;
}

const char *get_value(token t)
{
	assert_valid_token(t);
	return &(t->src[t->off]);
}

struct name_map {
	enum token_type type;
	char *text;
	size_t length;
};
struct name_map name_map_search(enum token_type type)
{
	struct name_map map[TOKEN_TYPE_COUNT - TOKEN_START] = {
	    {TOKEN_START, "Start", strlen("Start")},
	    {TOKEN_EOF, "Eof", strlen("Eof")},
	    {TOKEN_NUMBER, "Number", strlen("Number")},
	    {TOKEN_OPERATOR, "Operator", strlen("Operator")},
	    {TOKEN_LPAREN, "Open parenthesis", strlen("Open parenthesis")},
	    {TOKEN_RPAREN, "Close parenthesis", strlen("Close parenthesis")},
	    {TOKEN_SEMICOLON, "Semicolon", strlen("Semicolon")},
	    {TOKEN_IDENTIFIER, "Identifier", strlen("Identifier")},
	    {TOKEN_LET, "Let", strlen("Let")},
	    {TOKEN_ASSIGNMENT, "Assignment", strlen("Assignment")},
	    {TOKEN_FUNC, "Func", strlen("Func")},
	    {TOKEN_LCBRACE, "Left Curly Brace", strlen("Left Curly Brace")},
	    {TOKEN_RCBRACE, "Right Curly Brace", strlen("Right Curly Brace")},
	    {TOKEN_COMMA, "Comma", strlen("Comma")}};
	return map[type];
}

#define SEPERATOR " : "
/* Assumes that the out buffer can hold the result (is 2048*4 + 21 bytes). */
void token_print(token t, char *out)
{
	size_t use = 0;
	struct name_map lookup = name_map_search(t->type);

	memcpy(out + use, lookup.text, lookup.length);
	use += lookup.length;
	memcpy(out + use, SEPERATOR, strlen(SEPERATOR));
	use += strlen(SEPERATOR);
	memcpy(out + use, get_value(t), t->len);
	use += t->len;
	out[use++] = '\0';
}

size_t token_print_len(token t)
{
	struct name_map lookup = name_map_search(t->type);
	return lookup.length + strlen(SEPERATOR) + t->len + 1;
}
