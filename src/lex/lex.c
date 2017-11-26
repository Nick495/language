#include "lex.h"

/*
 * Unfortunately, there are no recursive typedefs in C, so we use a dummy.
 * Source: comp.lang.c FAQ 1.22 (http://www.c-faq.com/decl/recurfuncp.html)
*/
typedef int (*state_func)(struct lexer *); /* Dummy function pointer. */
typedef state_func (*state_func_ptr)(struct lexer *);

static state_func lex_start(struct lexer *);

struct lexer {		      /* Cumulative size in bytes */
	char *str;	    /*  8 */
	state_func_ptr state; /* 16 */
	char *cur;	    /* 24 */
	char *pos;	    /* 32 */
	int emitted;	  /* 40 */
	int inject_semi;      /* 48 */
	const char *in_name;  /* 56 */
	token holder;	 /* 64 (End of cache line */
};

struct lexer *lexer_make()
{
	struct lexer *l = malloc(sizeof *l);
	assert(l); /* TODO: Error handling */
	return l;
}

void lexer_free(struct lexer *l)
{
	assert(l);
	free(l);
}

void lexer_init(struct lexer *l, char *in, char *in_name)
{
	assert(l);
	l->state = lex_start;
	l->str = in;
	l->in_name = in_name;
	l->cur = l->pos = l->str;
	l->inject_semi = l->emitted = 0;
}

token lex_token(struct lexer *l, token prev)
{
	assert(l);
	l->holder = prev;
	while (l->state != NULL && !l->emitted) {
		l->state = (state_func_ptr)l->state(l);
	}
	l->emitted = 0;
	return l->holder;
}

static void emit_token(struct lexer *l, enum token_type type)
{
	l->emitted = 1;
	l->holder = token_make(type, l->str, l->pos - l->cur, l->cur - l->str,
			       l->holder);
	l->cur = l->pos;
	return;
}

static void erase_lexemes(struct lexer *l) { l->cur = l->pos; }

static char next(struct lexer *l)
{
	char c = *l->pos;
	if (c == '\\') { /* Allow skipping newlines with '\ \n' form */
		if (l->pos[1] == '\n') {
			l->pos += 1; /* Skip both characters */
		}
	}
	l->pos += 1;
	return c;
}

static void backup(struct lexer *l) { l->pos--; }

static void inject_semicolon(struct lexer *l)
{
	erase_lexemes(l);
	emit_token(l, TOKEN_SEMICOLON);
	l->inject_semi = 0;
}

static state_func lex_space(struct lexer *l)
{
	char c = next(l);
	while (isspace(c)) {
		if (c == '\n' && l->inject_semi) {
			inject_semicolon(l);
		}
		c = next(l);
	}
	backup(l);
	erase_lexemes(l);
	return (state_func)lex_start;
}

static state_func lex_number(struct lexer *l)
{
	char c = next(l);
	while (isdigit(c)) {
		c = next(l);
	}
	backup(l);
	emit_token(l, TOKEN_NUMBER);
	l->inject_semi = 1;
	return (state_func)lex_start;
}

struct keyword {
	const char *name;
	size_t length;
	enum token_type token_type;
};
static struct keyword keywords[] = {{"let", 3, TOKEN_LET},
				    {":=", 2, TOKEN_ASSIGNMENT},
				    {"+", 1, TOKEN_OPERATOR},
				    {NULL, 0, TOKEN_EOF}};
static state_func lex_identifier(struct lexer *l)
{
	char c = next(l);
	enum token_type type = TOKEN_IDENTIFIER;
	while (!isspace(c)) {
		c = next(l);
	}
	backup(l);

	for (struct keyword *k = keywords; k->name != NULL; ++k) {
		if ((size_t)(l->pos - l->cur) == k->length &&
		    !memcmp(l->cur, k->name, k->length)) {
			type = k->token_type;
			break;
		}
	}
	emit_token(l, type);
	l->inject_semi = 1;
	return (state_func)lex_start;
}

static state_func lex_start(struct lexer *l)
{
	char c = next(l);
	if (isspace(c)) {
		backup(l);
		return (state_func)lex_space;
	} else if (isdigit(c)) {
		backup(l);
		return (state_func)lex_number;
	}
	switch (c) {
	case '(':
		emit_token(l, TOKEN_LPAREN);
		return (state_func)lex_start;
	case ')':
		emit_token(l, TOKEN_RPAREN);
		return (state_func)lex_start;
	case ';':
		emit_token(l, TOKEN_SEMICOLON);
		return (state_func)lex_start;
	case '\0':
		if (c == '\n' && l->inject_semi) {
			inject_semicolon(l);
		}
		l->cur = l->pos; /* Don't duplicate null terminators. */
		emit_token(l, TOKEN_EOF);
		return NULL;
	default:
		backup(l);
		return (state_func)lex_identifier;
	}
}
