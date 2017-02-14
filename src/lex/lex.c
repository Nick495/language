#include "lex.h"

struct lexer {
	char *file_name;
	FILE *input;
	string lexeme;
	int outfd;
	token token_cache;
};

struct lexer *lexer_make(char *file_name, FILE *in, int out)
{
	struct lexer *l = malloc(sizeof *l);
	assert(l); /* TODO: Error handling */
	memset(l, 0, sizeof *l);
	l->file_name = file_name;
	l->input = in;
	l->outfd = out;
	l->lexeme = string_make();
	return l;
}

void lexer_free(struct lexer *l)
{
	assert(l);
	fclose(l->input);
	close(l->outfd);
	free_token(l->token_cache);
	string_free(l->lexeme);
	free(l);
}

static void emit_token(struct lexer *l, enum token_type type)
{
	/* Cache allocated tokens inside the lexer to avoid malloc()s */
	l->token_cache = make_token(type, get_text(l->lexeme),
			get_length(l->lexeme) + 1, l->token_cache);
	assert(l->token_cache); /* TODO: Error handling */
	assert(!write_token(l->token_cache, l->outfd)); /* TODO: Error handling */
	empty_string(l->lexeme);
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

/*
 * Unfortunately there are no recursive function definitions in C, so subvert
 * the typesystem and use void*
*/
typedef void* (*state_func)(struct lexer *);

/* Statefunc prototyes (since they refer to one another, must forward declare */
static void *lex_start(struct lexer *);

static void *lex_space(struct lexer *l)
{
	int c = next(l);
	while (isspace(c)) {
		c = next(l);
	}
	backup(l, c);
	return (void *)lex_start;
}

static void *lex_number(struct lexer *l)
{
	int c = next(l);
	while (isdigit(c)) {
		string_append_char(l->lexeme, c);
		c = next(l);
	}
	backup(l, c);
	emit_token(l, TOKEN_NUMBER);
	return (void *)lex_start;
}

static void *lex_operator(struct lexer *l)
{
	int c = next(l);
	if (c != '+') {
		return NULL; /* TODO: Error handling */
	}
	string_append_char(l->lexeme, c);
	emit_token(l, TOKEN_OPERATOR);
	return (void *)lex_start;
}

static void *lex_start(struct lexer *l)
{
	for (;;) {
		int c = next(l);
		if (isspace(c)) {
			backup(l, c);
			return (void *)lex_space;
		} else if (isdigit(c)) {
			backup(l, c);
			return (void *)lex_number;
		} else if (c == '+') {
			backup(l, c);
			return (void *)lex_operator;
		} else if (c == '(') {
			string_append_char(l->lexeme, '(');
			emit_token(l, TOKEN_LPAREN);
		} else if (c == ')') {
			string_append_char(l->lexeme, ')');
			emit_token(l, TOKEN_RPAREN);
		} else if (c == EOF) {
			string_append_cstr(l->lexeme, "End of file");
			emit_token(l, TOKEN_EOF);
			return NULL;
		} else {
			fprintf(stderr, "Error. Bad character: %c\n", c);
			return NULL; /* TODO: Error handling */
		}
	}
}

int lex(FILE *in, int out)
{
	struct lexer *l = lexer_make("stdin", in, out);
	for (state_func state = lex_start; state != NULL;) {
		state = (state_func) state(l);
	}
	lexer_free(l);
	return 0;
}
