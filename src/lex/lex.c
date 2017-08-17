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
	char *file_name;
	FILE *input;
	string lexeme;
	token token_cache;
	state_func_ptr state;
	int emitted;
};

struct lexer* lexer_make(char *file_name, FILE *in)
{
	struct lexer *l = mem_alloc(sizeof *l);
	assert(l); /* TODO: Error handling */
	memset(l, 0, sizeof *l);
	l->file_name = file_name;
	l->input = in;
	l->lexeme = string_make();
	l->state = lex_start;
	return l;
}

void lexer_free(struct lexer *l)
{
	assert(l);
	fclose(l->input);
	free_token(l->token_cache);
	string_free(l->lexeme);
	free(l);
}

static void emit_token(struct lexer *l, enum token_type type)
{
	/* Cache allocated tokens inside the lexer to avoid mem_alloc()s */
	/* TODO: Remove copy here by refactoring empty_string & make_token? */
	l->token_cache = make_token(
		type,
		get_text(l->lexeme),
		get_length(l->lexeme) + 1,
		l->token_cache
	);
	assert(l->token_cache); /* TODO: Error handling */
	empty_string(l->lexeme);
	l->emitted = 1;
	return;
}

static int next(struct lexer *l)
{
	return fgetc(l->input);
}

static int peek(struct lexer *l)
{
	int c = fgetc(l->input);
	ungetc(c, l->input);
	return c;
}

static void backup(struct lexer *l, int c)
{
	ungetc(c, l->input);
}

static state_func lex_space(struct lexer *l)
{
	int c = next(l);
	while (isspace(c)) {
		c = next(l);
	}
	backup(l, c);
	return (state_func)lex_start;
}

static state_func lex_number(struct lexer *l)
{
	int c = next(l);
	while (isdigit(c)) {
		string_append_char(l->lexeme, c);
		c = next(l);
	}
	backup(l, c);
	emit_token(l, TOKEN_NUMBER);
	return (state_func)lex_start;
}

static state_func lex_operator(struct lexer *l)
{
	int c = next(l);
	if (c != '+') {
		return NULL; /* TODO: Error handling */
	}
	string_append_char(l->lexeme, c);
	emit_token(l, TOKEN_OPERATOR);
	return (state_func)lex_start;
}

static state_func lex_start(struct lexer *l)
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
		string_append_char(l->lexeme, '(');
		emit_token(l, TOKEN_LPAREN);
		return (state_func)lex_start;
	} else if (c == ')') {
		string_append_char(l->lexeme, ')');
		emit_token(l, TOKEN_RPAREN);
		return (state_func)lex_start;
	} else if (c == EOF) {
		string_append_cstr(l->lexeme, "End of file");
		emit_token(l, TOKEN_EOF);
		return NULL;
	} else {
		fprintf(stderr, "Error. Bad character: %c\n", c);
		return NULL; /* TODO: Error handling */
	}
}

token lex_token(struct lexer *l)
{
	assert(l);
	while (l->state != NULL && !l->emitted) {
		l->state = (state_func_ptr) l->state(l);
	}
	l->emitted = 0;
	return make_token(
		get_type(l->token_cache),
		get_value(l->token_cache),
		strlen(get_value(l->token_cache)),
		NULL
	);
}
