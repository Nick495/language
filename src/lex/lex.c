#include "lex.h"

struct lexer; /* Must forward declare for the sake of typedefs. */

/*
 * Unfortunately, there are no recursive typedefs in C, so we use a dummy.
 * Source: comp.lang.c FAQ 1.22 (http://www.c-faq.com/decl/recurfuncp.html)
*/
typedef int (*state_func)(struct lexer *); /* Dummy function pointer. */
typedef state_func (*state_func_ptr)(struct lexer *);

/* Statefunc prototyes- since they refer to each other, must forward declare */
static state_func lex_start(struct lexer *);

struct lexer {
	FILE* input;
	state_func_ptr state;
	size_t token_len;
	enum token_type token_type;
	int emitted;
	const char* file_name;
	char token_str[2048];
};

struct lexer* lexer_make(const char* file_name, FILE *in)
{
	struct lexer* l = mem_alloc(sizeof *l + token_size());
	assert(l); /* TODO: Error handling */
	memset(l, 0, sizeof *l + token_size());
	l->file_name = file_name;
	l->input = in;
	l->state = lex_start;
	return l;
}

void lexer_free(struct lexer* l)
{
	assert(l);
	fclose(l->input);
	free(l);
}

static void append_char(struct lexer* l, const char c)
{
	l->token_str[l->token_len++] = c;
	assert(l->token_len < 2047); /* TODO: Error handling. */
}

static void append_cstr(struct lexer* l, const char* str)
{
	const size_t str_len = strlen(str);
	assert(l->token_len + str_len < 2047); /* TODO: Error handling. */
	memcpy(&l->token_str[l->token_len], str, str_len);
	l->token_len += str_len;
}

static void emit_token(struct lexer* l, enum token_type type)
{
	/* Cache allocated tokens inside the lexer to avoid mem_alloc()s */
	/* TODO: Remove copy here by refactoring empty_string & token_make? */
	l->token_type = type;
	l->emitted = 1;
	return;
}

static void cleanup_token_cache(struct lexer* l)
{
	l->token_len = 0;
	l->emitted = 0;
}

static int next(struct lexer* l)
{
	return getc_unlocked(l->input);
}

static void backup(struct lexer* l, int c)
{
	ungetc(c, l->input);
}

static state_func lex_space(struct lexer* l)
{
	int c = next(l);
	while (isspace(c)) {
		c = next(l);
	}
	backup(l, c);
	return (state_func)lex_start;
}

static state_func lex_number(struct lexer* l)
{
	int c = next(l);
	while (isdigit(c)) {
		append_char(l, c);
		c = next(l);
	}
	backup(l, c);
	emit_token(l, TOKEN_NUMBER);
	return (state_func)lex_start;
}

static state_func lex_operator(struct lexer* l)
{
	int c = next(l);
	if (c != '+') {
		return NULL; /* TODO: Error handling */
	}
	append_char(l, c);
	emit_token(l, TOKEN_OPERATOR);
	return (state_func)lex_start;
}

static state_func lex_start(struct lexer* l)
{
	int c = next(l);
	if (isspace(c)) {
		backup(l, c);
		return (state_func)lex_space;
	} else if (isdigit(c)) {
		backup(l, c);
		return (state_func)lex_number;
	} else if (c == '+') {
		backup(l, c);
		return (state_func)lex_operator;
	} else if (c == '(') {
		append_char(l, '(');
		emit_token(l, TOKEN_LPAREN);
		return (state_func)lex_start;
	} else if (c == ')') {
		append_char(l, '(');
		emit_token(l, TOKEN_RPAREN);
		return (state_func)lex_start;
	} else if (c == EOF) {
		append_cstr(l, "End of file");
		emit_token(l, TOKEN_EOF);
		return NULL;
	} else {
		fprintf(stderr, "Error. Bad character: %c\n", c);
		return NULL; /* TODO: Error handling */
	}
}

token lex_token(struct lexer* l, token prev)
{
	token t;
	assert(l);
	while (l->state != NULL && !l->emitted) {
		l->state = (state_func_ptr) l->state(l);
	}
	t = token_make(
		l->token_type,
		l->token_str,
		l->token_len,
		prev
	);
	cleanup_token_cache(l);
	return t;
}
