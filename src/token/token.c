#include "token.h"

struct token_ {
	enum token_type type;
	size_t size;
	size_t alloced;
	char *value;
};

static void assert_valid_token(token t)
{
	assert(t);
	assert(t->value);
}

/* Handles null case for internal code reuse. */
void free_token(token t)
{
	if (t) {
		mem_dealloc(t->value);
	}
	mem_dealloc(t);
}

static token alloc_token(size_t len, token prev)
{
	token t;
	if (prev) {
		t = prev;
	} else {
		t = mem_alloc(sizeof *t);
		if (!t) goto fail_mem_alloc_token;
		t->value = NULL;
		t->alloced = 0;
	}
	if (t->alloced < len) {
		char *new = mem_realloc(t->value, sizeof t->value * len);
		if (!new) goto fail_mem_alloc_value;
		t->value = new;
		t->alloced = len;
	}
	assert_valid_token(t);
	return t;

	mem_dealloc(t->value);
fail_mem_alloc_value:
fail_mem_alloc_token:
	free_token(t);
	return NULL;
}

token make_token(enum token_type type, char *s, size_t slen, token prev)
{
	slen += 1; /* Include '\0' */
	token t = alloc_token(slen, prev);
	if (!t) goto fail_alloc_token;
	t->type = type;
	t->size = slen;
	memcpy(t->value, s, slen);
	assert_valid_token(t);
	return t;

	free_token(t);
fail_alloc_token:
	return NULL;
}

enum token_type get_type(token t)
{
	assert_valid_token(t);
	return t->type;
}

char *get_value(token t)
{
	assert_valid_token(t);
	return t->value;
}
