#include "parse.h"

#define LOOKAHEAD 2 /* Must be > 1 */
struct Parser {
	size_t buf_read;
	size_t buf_write;
	struct lexer *lex;
	token buf[LOOKAHEAD];
	char* input_name;
};

struct Parser* parser_make()
{
	struct Parser* p = mem_alloc(sizeof *p + token_size() * LOOKAHEAD);
	assert(p); /* TODO: Error handling. */
	p->lex = lexer_make();
	assert(p->lex); /* TODO: Error handling. */
	p->buf_read = 0;
	p->buf_write = 0;
	for (size_t i = 0; i < LOOKAHEAD; ++i) {
		p->buf[i] = (token)((char *)p + sizeof *p + token_size() * i);
	}
	return p;
}

void parser_free(struct Parser* p)
{
	if (p) {
		lexer_free(p->lex);
	}
	free(p);
}

static int is_empty(struct Parser* p)
{
	return p->buf_write == p->buf_read;
}

static token token_pop(struct Parser* p)
{
	token t = NULL;
	t = token_copy(t, p->buf[p->buf_read]);
	p->buf_read = (p->buf_read + 1) % LOOKAHEAD;
	return t;
}

static void token_push(struct Parser* p)
{
	p->buf[p->buf_write] = lex_token(p->lex, p->buf[p->buf_write]);
	p->buf_write = (p->buf_write + 1) % LOOKAHEAD;
}

static token peek(struct Parser *p)
{
	if (is_empty(p)) {
		token_push(p);
	}
	return p->buf[p->buf_read];
}

static token next(struct Parser *p)
{
	token t = NULL;
	if (is_empty(p)) {
		t = lex_token(p->lex, t);
	} else {
		t = token_pop(p);
	}
	assert(t);
	return t;
}

ASTNode Expr(struct Parser *, token);
ASTNode Op(struct Parser *, token);

//	expr
//		operand
//		operand binop expr
ASTNode Expr(struct Parser *p, token t)
{
	ASTNode expr = Op(p, t);
	switch (get_type(peek(p))) {
	case TOKEN_EOF: /* FALLTHRU */
	case TOKEN_RPAREN:
		return expr;
	case TOKEN_OPERATOR: { /* Dyadic (binop) */
		ASTNode res;
		token t = next(p);
		res = make_binop(expr, get_value(t), Expr(p, next(p)));
		token_free(t);
		return res;
	}
	default:
		printf("DEBUG: %s\n", get_value(peek(p)));
		assert(0); /* TODO: Error handling */
		return NULL;
	}
}

/* Note, can seperate into two functions. */
ASTNode NumberOrVector(struct Parser *p, token t)
{
	ASTNode res;
	if (get_type(peek(p)) != TOKEN_NUMBER) {
		res = make_number(get_value(t));
		token_free(t);
		return res;
	}
	res = make_vector(get_value(t));
	token_free(t);
	while (get_type(peek(p)) == TOKEN_NUMBER) {
		t = next(p);
		res = extend_vector(res, get_value(t));
		token_free(t);
	}
	return res;
}

// Grammar from Rob Pike's talk
//	operand
//		( Expr )
//		( Expr ) [ Expr ]...
//		operand
//		number
//		unop Expr
ASTNode Op(struct Parser *p, token t)
{
	ASTNode op;
	switch(get_type(t)) {
	case TOKEN_LPAREN:
		op = Expr(p, next(p));
		token_free(t);
		t = next(p);
		assert(get_type(t) == TOKEN_RPAREN); /* TODO: Error handling. */
		token_free(t);
		break;
	case TOKEN_NUMBER:
		op = NumberOrVector(p, t);
		break;
	case TOKEN_OPERATOR:
		op = make_unop(get_value(t), Expr(p, next(p)));
		token_free(t);
		break;
	default:
		printf("DEBUG: %s\n", get_value(t));
		token_free(t);
		assert(0); /* TODO: Error handling */
		return NULL;
	};
	return op; /* TODO: Indexing. */
}

ASTNode parse(struct Parser* p, char* in, char* in_name)
{
	assert(p);
	assert(in);

	lexer_init(p->lex, in, in_name);
	p->input_name = in_name;
	return Expr(p, next(p));
}
