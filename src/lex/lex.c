#include "lex.h"

/*
 * Unfortunately, there are no recursive typedefs in C, so we use a dummy.
 * Source: comp.lang.c FAQ 1.22 (http://www.c-faq.com/decl/recurfuncp.html)
*/
typedef int (*state_func)(struct lexer *); /* Dummy function pointer. */
typedef state_func (*state_func_ptr)(struct lexer *);

static state_func lex_start(struct lexer *);

/* Max token length is 2048 characters */
struct lexer {
	char* str;
	state_func_ptr state;
	size_t token_len;
	enum token_type token_type;
	int emitted;
	const char* in_name;
	char token_str[2048];
};

struct lexer* lexer_make()
{
	struct lexer* l = mem_alloc(sizeof *l + token_size());
	assert(l); /* TODO: Error handling */
	memset(l, 0, sizeof *l + token_size());
	l->state = lex_start;
	return l;
}

void lexer_free(struct lexer* l)
{
	assert(l);
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

/*
 * Turns the token_str into a legitimate c string by adding a null character.
 * Factoring this out saves one write per append_char or append_cstr.
*/
static void fixup_token(struct lexer* l)
{
	l->token_str[l->token_len] = '\0';
}

static void emit_token(struct lexer* l, enum token_type type)
{
	/* Cache allocated tokens inside the lexer to avoid mem_alloc()s */
	/* TODO: Remove copy here by refactoring empty_string & token_make? */
	l->token_type = type;
	fixup_token(l);
	l->emitted = 1;
	return;
}

static void cleanup_token_cache(struct lexer* l)
{
	l->token_len = 0;
	l->emitted = 0;
}

static char next(struct lexer* l)
{
	return *(l->str++);
}

static void backup(struct lexer* l, char c)
{
	l->str--;
}

static state_func lex_space(struct lexer* l)
{
	char c = next(l);
	while (isspace(c)) {
		c = next(l);
	}
	backup(l, c);
	return (state_func)lex_start;
}

static state_func lex_number(struct lexer* l)
{
	char c = next(l);
	while (isdigit(c)) {
		append_char(l, c);
		c = next(l);
	}
	backup(l, c);
	emit_token(l, TOKEN_NUMBER);
	return (state_func)lex_start;
}

static state_func lex_identifier(struct lexer* l)
{
	char c = next(l);
	while (!isspace(c)) {
		append_char(l, c);
		c = next(l);
	}
	/* No backup. Consume the space. */
	fixup_token(l);
	if (!strcmp(l->token_str, "let")) { /* Found let */
		emit_token(l, TOKEN_LET);
	} else if (!strcmp(l->token_str, "+")) {
		emit_token(l, TOKEN_OPERATOR);
	} else {
		emit_token(l, TOKEN_IDENTIFIER);
	}
	return (state_func)lex_start;
}

static state_func lex_start(struct lexer* l)
{
	char c = next(l);
	if (isspace(c)) {
		backup(l, c);
		return (state_func)lex_space;
	} else if (isdigit(c)) {
		backup(l, c);
		return (state_func)lex_number;
	} else if (c == '(') {
		append_char(l, '(');
		emit_token(l, TOKEN_LPAREN);
		return (state_func)lex_start;
	} else if (c == ')') {
		append_char(l, '(');
		emit_token(l, TOKEN_RPAREN);
		return (state_func)lex_start;
	} else if (c == ';') {
		append_char(l, ';');
		emit_token(l, TOKEN_SEMICOLON);
		return (state_func)lex_start;
	} else if (c == '\0') {
		append_cstr(l, "End of string");
		emit_token(l, TOKEN_EOF);
		return NULL;
	} else { /* Has to be an identifier */
		backup(l, c);
		return (state_func)lex_identifier;
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

void lexer_init(struct lexer* l, char* in, char* in_name)
{
	assert(l);
	l->state = lex_start;
	l->token_len = l->emitted = 0;
	l->str = in;
	l->in_name = in_name;
}
