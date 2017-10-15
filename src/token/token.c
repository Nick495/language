#include "token.h"

#define MAX_SIZE 2048
struct token_ {
	enum token_type type;
	size_t size;
	char value[MAX_SIZE];
};

static void assert_valid_token(token t)
{
	assert(t);
	assert(strlen(t->value) == t->size);
}

/* Handles null case for internal code reuse. */
void token_free(token t)
{
	mem_dealloc(t);
}

static token alloc_token(token prev)
{
	token t;
	if (prev) {
		t = prev;
	} else {
		t = mem_alloc(sizeof *t);
		if (!t) goto fail_mem_alloc_token;
	}
	t->value[0] = '\0';
	t->size = 0;
	assert_valid_token(t);
	return t;

fail_mem_alloc_token:
	token_free(t);
	return NULL;
}

token token_make(enum token_type type, const char* s, size_t slen, token prev)
{
	token t;
	assert(slen < MAX_SIZE); /* TODO: Error handling. */
	t = alloc_token(prev);
	if (!t) goto fail_alloc_token;
	t->type = type;
	t->size = slen;
	memcpy(t->value, s, slen + 1);
	assert_valid_token(t);
	return t;

	token_free(t);
fail_alloc_token:
	return NULL;
}

token token_copy(token dst, token src)
{
	assert_valid_token(src);
	dst = alloc_token(dst);
	assert_valid_token(dst); /* TODO: Error handling */
	memcpy(dst, src, sizeof *src);
	return dst;
}

enum token_type get_type(token t)
{
	assert_valid_token(t);
	return t->type;
}

char* get_value(token t)
{
	assert_valid_token(t);
	return t->value;
}

/* Assumes that the out buffer can hold the result (is 2048*4 + 21 bytes). */
void token_print(token t, char* out)
{
	char* name = NULL;
	const char* sep = " : ";
	const char* value = get_value(t);
	const size_t seplen = strlen(sep);
	const size_t valuelen = strlen(value);
	size_t use = 0;
	size_t namelen = 0;
	switch(get_type(t)) {
	case TOKEN_START:
		name = "Start";
		break;
	case TOKEN_EOF:
		name = "Eof";
		break;
	case TOKEN_NUMBER:
		name = "Number";
		break;
	case TOKEN_OPERATOR:
		name = "Operator";
		break;
	case TOKEN_LPAREN:
		name = "Open parenthesis";
		break;
	case TOKEN_RPAREN:
		name = "Close parenthesis";
		break;
	case TOKEN_SEMICOLON:
		name = "Semicolon";
		break;
	case TOKEN_IDENTIFIER:
		name = "Identifier";
		break;
	case TOKEN_LET:
		name = "Let";
		break;
	case TOKEN_ASSIGNMENT:
		name = "Assignment";
		break;
	};
	namelen = strlen(name);
	memcpy(out + use, name, namelen);
	use += namelen;
	memcpy(out + use, sep, seplen);
	use += seplen;
	memcpy(out + use, value, valuelen + 1);
	use += valuelen + 1;
	assert(use <= 2048 * 4 + 21);
}

size_t token_size()
{
	token t;
	return sizeof *t;
}
