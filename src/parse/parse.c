#include "parse.h"

#define BUF_CAP 2 /* Must be > 1 */
struct Parser {
	size_t buf_read;
	size_t buf_write;
	struct lexer *lex;
	token buf[BUF_CAP];
};

struct Parser *parser_make(FILE* in)
{
	struct Parser* p = mem_alloc(sizeof *p + token_size() * BUF_CAP);
	assert(p); /* TODO: Error handling. */
	p->lex = lexer_make("stdin", in);
	assert(p->lex); /* TODO: Error handling. */
	p->buf_read = 0;
	p->buf_write = 0;
	for (size_t i = 0; i < BUF_CAP; ++i) {
		p->buf[i] = (token)((char *)p + sizeof *p + token_size() * i);
	}
	return p;
}

void parser_free(struct Parser *p)
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
	p->buf_read = (p->buf_read + 1) % BUF_CAP;
	return t;
}

static void token_push(struct Parser* p)
{
	p->buf[p->buf_write] = lex_token(p->lex, p->buf[p->buf_write]);
	p->buf_write = (p->buf_write + 1) % BUF_CAP;
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
		token t = next(p); /* FIXME: Send binop token t directly. */
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
	while (get_type(peek(p)) == TOKEN_NUMBER) {
		token_free(t);
		t = next(p);
		res = extend_vector(res, get_value(t));
	}
	token_free(t);
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
	ASTNode expr;
	switch(get_type(t)) {
	case TOKEN_LPAREN:
		expr = Expr(p, next(p));
		token_free(t);
		t = next(p);
		assert(get_type(t) == TOKEN_RPAREN); /* TODO: Error handling. */
		token_free(t);
		break;
	case TOKEN_NUMBER:
		expr = NumberOrVector(p, t);
		break;
	case TOKEN_OPERATOR:
		expr = make_unop(get_value(t), Expr(p, next(p)));
		break;
	default:
		printf("DEBUG: %s\n", get_value(t));
		free(t);
		assert(0); /* TODO: Error handling */
		return NULL;
	};
	return expr; /* TODO: Indexing. */
}

int parse(FILE *in, FILE *out)
{
	struct Parser *p = parser_make(in);
	assert(p); /* TODO: Error handling */
	ASTNode tree = Expr(p, next(p));
	Value result = Eval(tree);
	char* res_string = value_stringify(result);
	fprintf(out, "%s\n", res_string);
	free(res_string);
	value_free(result);
	free_node(tree);
	parser_free(p);
	return 0;
}
