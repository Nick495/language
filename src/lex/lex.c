#include "lex.h"

/*
 * Unfortunately, there are no recursive typedefs in C, so we use a dummy.
 * Source: comp.lang.c FAQ 1.22 (http://www.c-faq.com/decl/recurfuncp.html)
*/
typedef int (*state_func)(struct lexer *); /* Dummy function pointer. */
typedef state_func (*state_func_ptr)(struct lexer *);

static state_func lex_start(struct lexer *);

/* Max token length is 2048 characters. UTF8 characters are max 4 bytes. */
enum MAX_TOKEN_LEN { MAX_TOKEN_LEN = 2048 };
enum RUNE_SIZE { RUNE_SIZE = 4 };

struct lexer {
	char* str;
	unsigned int width;
	state_func_ptr state;
	char* cur;
	char* pos;
	int emitted;
	enum token_type token_type;
	size_t token_len;
	const char* in_name;
	char token_str[MAX_TOKEN_LEN * RUNE_SIZE];
};

struct lexer* lexer_make()
{
	struct lexer* l = mem_alloc(sizeof *l + token_size());
	assert(l); /* TODO: Error handling */
	return l;
}

void lexer_free(struct lexer* l)
{
	assert(l);
	free(l);
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
	l->emitted = 0;
	return t;
}

void lexer_init(struct lexer* l, char* in, char* in_name)
{
	assert(l);
	l->state = lex_start;
	l->str = in;
	l->in_name = in_name;
	l->cur = l->pos = l->str;
	l->token_len = l->emitted = 0;
}

static void emit_token(struct lexer* l, enum token_type type)
{
	/* TODO: Remove copy here by refactoring empty_string & token_make? */
	l->token_type = type;
	l->token_len = l->pos - l->cur;
	memcpy(l->token_str, l->cur, l->token_len);
	l->token_str[l->token_len] = '\0';
	l->emitted = 1;
	l->cur = l->pos;
	return;
}

/* Cheeky function needed since the EOS token is a null terminator itself. */
static void emit_eos(struct lexer *l, enum token_type type)
{
	l->cur = l->pos;
	emit_token(l, type);
}

static void erase_lexemes(struct lexer *l)
{
	l->cur = l->pos;
}

static unsigned int codepoint_size(unsigned char c)
{
	unsigned int size;
	if ((c & 0x80) == 0x00) {
		size = 1;
	} else if ((c & 0xE0) == 0xC0) {
		size = 2;
	} else if ((c & 0xF0) == 0xE0) {
		size = 3;
	} else if ((c & 0xF8) == 0xF0) {
		size = 4;
	} else { /* Unknown unicode character encountered. */
		assert(0);
	}
	return size;
}

static void verify_utf8(unsigned char *codepoint, unsigned int size)
{
	unsigned int i;
	for (i = 1; i < size; ++i) {
		assert(((unsigned char) codepoint[i] & 0xC0) == 0x80);
	}
}

static char next(struct lexer* l)
{
	char c = *l->pos;
	l->width = codepoint_size(c);
	verify_utf8((unsigned char *)l->pos, l->width);
	l->pos += l->width;
	/* We need one character for the null terminator. */
	assert((l->pos - l->cur) < (MAX_TOKEN_LEN - 1) * RUNE_SIZE);
	return c;
}

static void backup(struct lexer* l)
{
	l->pos -= l->width;
	l->width = 0;
}

static state_func lex_space(struct lexer* l)
{
	char c = next(l);
	while (isspace(c)) {
		c = next(l);
	}
	backup(l);
	erase_lexemes(l);
	return (state_func)lex_start;
}

static state_func lex_number(struct lexer* l)
{
	char c = next(l);
	while (isdigit(c)) {
		c = next(l);
	}
	backup(l);
	emit_token(l, TOKEN_NUMBER);
	return (state_func)lex_start;
}

static state_func lex_identifier(struct lexer* l)
{
	char c = next(l);
	while (!isspace(c)) {
		c = next(l);
	}
	backup(l);
	/* TODO: Find a better way to deal with keywords. */
	if (!memcmp(l->cur, "let", sizeof "let")) { /* Found let */
		emit_token(l, TOKEN_LET);
	} else if (memcmp(l->cur, "+", sizeof "+")) {
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
		backup(l);
		return (state_func)lex_space;
	} else if (isdigit(c)) {
		backup(l);
		return (state_func)lex_number;
	} else if (c == '(') {
		emit_token(l, TOKEN_LPAREN);
		return (state_func)lex_start;
	} else if (c == ')') {
		emit_token(l, TOKEN_RPAREN);
		return (state_func)lex_start;
	} else if (c == ';') {
		emit_token(l, TOKEN_SEMICOLON);
		return (state_func)lex_start;
	} else if (c == '\0') {
		emit_eos(l, TOKEN_EOF);
		return NULL;
	} else { /* Has to be an identifier */
		backup(l);
		return (state_func)lex_identifier;
	}
}
