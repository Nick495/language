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

size_t token_size()
{
	token t;
	return sizeof *t;
}
