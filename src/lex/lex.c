#include "lex.h"

struct lexer {
	char *name;
	FILE *input;
	size_t start;
	size_t position;
	size_t width;
	string lexeme;
	int outfd;
};

struct lexer *make_lexer(char *name, FILE *in, int out)
{
	struct lexer *l = malloc(sizeof *l);
	assert(l); /* TODO: Error handling */
	memset(l, 0, sizeof *l);
	l->name = name;
	l->input = in;
	l->outfd = out;
	l->lexeme = make_string();
	return l;
}

void free_lexer(struct lexer *l)
{
	assert(l);
	fclose(l->input);
	close(l->outfd);
	free_string(l->lexeme);
	free(l);
}

static void emit_token(struct lexer *l, enum token_type type)
{
	token t = make_token(type, get_text(l->lexeme), get_length(l->lexeme) + 1);
	assert(t); /* TODO: Error handling */
	assert(!write_token(t, l->outfd)); /* TODO: Error handling */
	empty_string(l->lexeme);
	free(t);
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
			assert(1); /* TODO: Error handling. */
			return NULL;
		}
	}
}

int lex(FILE *in, int out)
{
	struct lexer *l = make_lexer("stdin", in, out);
	for (state_func state = lex_start; state != NULL;) {
		state = (state_func) state(l);
	}
	free_lexer(l);
	return 0;
}
