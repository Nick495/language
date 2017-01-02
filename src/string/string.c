#include "string.h"

struct string_ {
	size_t len;
	size_t cap;
	char *text;
};

static void assert_valid_string(string s)
{
	assert(s);
	assert(s->len <= s->cap);
	assert(s->text);
	assert(s->text[s->len] == '\0');
}

void empty_string(string s)
{
	assert(s);
	s->len = 0;
	s->text[s->len] = '\0';
	assert_valid_string(s);
}

void free_string(string s)
{
	if (s) {
		if (s->text) {
			free(s->text);
		}
		free(s);
	}
}

size_t get_length(string s)
{
	assert_valid_string(s);
	return s->len - 1; /* Skip '\0' for strlen() compatibility */
}

char *get_text(string s)
{
	assert_valid_string(s);
	return s->text;
}

static int enlarge_string(string s, size_t newcap)
{
	char *new_text = realloc(s->text, sizeof *s->text * newcap);
	if (!new_text) {
		return 1;
	}
	s->cap = newcap;
	s->text = new_text;
	return 0;
}

enum {DEFAULT = 100, GROWTH = 2};
int string_append_char(string s, char c)
{
	assert_valid_string(s);
	if (s->len == s->cap) {
		int rc = enlarge_string(s, s->cap * GROWTH);
		if (rc) {
			return rc;
		}
	}
	assert(s->len < s->cap);
	s->text[s->len++] = c;
	s->text[s->len] = '\0';
	assert_valid_string(s);
	return 0;
}

int string_append_cstr(string s, char *t)
{
	assert_valid_string(s);
	size_t newcap = s->cap, slen = s->len, tlen = strlen(t);
	while (tlen + slen >= newcap) {
		newcap *= GROWTH;
	}
	if (newcap > s->cap) {
		int rc = enlarge_string(s, newcap);
		if (rc) {
			return rc;
		}
	}
	assert(slen + tlen < newcap);
	/* Copy 't' including null terminator. */
	memmove(s->text + sizeof s->text[0] * slen, t, (tlen + 1) * sizeof *t);
	s->len += tlen;
	assert_valid_string(s);
	return 0;
}

string make_string(void)
{
	string s = malloc(sizeof *s);
	if (!s) goto fail_malloc_string;
	s->text = malloc(sizeof *s->text * DEFAULT);
	if (!s->text) goto fail_malloc_text;

	empty_string(s);
	s->cap = DEFAULT;
	return s;

fail_malloc_text:
fail_malloc_string:
	free_string(s);
	return NULL;
}
