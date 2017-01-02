#include "token.h"

struct token_ {
	size_t size;
	enum token_type type;
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
		free(t->value);
	}
	free(t);
}

static token alloc_token(size_t len)
{
	token t = malloc(sizeof *t);
	if (!t) goto fail_malloc_token;
	t->value = malloc(sizeof t->value * len);
	if (!t->value) goto fail_malloc_value;
	return t;

fail_malloc_value:
fail_malloc_token:
	free_token(t);
	return NULL;
}

token make_token(enum token_type type, char *s, size_t slen)
{
	slen += 1; /* Include '\0' */
	token t = alloc_token(slen);
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

static void serialize_int(size_t a, int out)
{
	uint64_t val = a;
	unsigned char buf[sizeof val];
	for (size_t i = 0; i < sizeof buf; ++i) {
		buf[i] = val << (i * 8);
	}
	/*TODO: Error handling */
	assert(write(out, buf, sizeof buf) == (ssize_t)sizeof buf);
}

int write_token(token t, int out)
{
	assert_valid_token(t);
	serialize_int(t->size, out);
	serialize_int(t->type, out);
	/* TODO: Error handling */
	assert(write(out, t->value, t->size) == (ssize_t)t->size);
	return 0;
}

static size_t deserialize_int(int in)
{
	uint64_t val = 0;
	unsigned char buf[sizeof val];
	assert(read(in, buf, sizeof buf) == sizeof buf);/* TODO:Error handling */
	for (size_t i = 0; i < sizeof buf; ++i) {
		val |= (uint64_t) (buf[i] << (i * 8));
	}
	return (size_t) val;
}

token read_token(int in)
{
	const size_t tsize = deserialize_int(in);
	token t = alloc_token(tsize);
	if (!t) goto fail_alloc_token;
	t->size = tsize;
	t->type = (enum token_type) deserialize_int(in);
	/* TODO: Error handling. */
	assert(read(in, t->value, t->size) == (ssize_t)t->size);
	return t;

	free_token(t);
fail_alloc_token:
	return NULL;
}
