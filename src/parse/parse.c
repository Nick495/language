#include "parse.h"

#define BUF_CAP 1
struct Parser {
	int infd;
	token last_popped;
	size_t buf_use;
	size_t buf_cap;
	token buf[BUF_CAP];
};

struct Parser *parser_make(int in)
{
	struct Parser *p = mem_alloc(sizeof *p);
	assert(p); /* TODO: Error handling. */
	p->infd = in;
	p->buf_use = 0;
	p->buf_cap = BUF_CAP;
	for (size_t i = 0; i < BUF_CAP; ++i) {
		p->buf[i] = NULL;
	}
	p->last_popped = NULL;
	return p;
}

void parser_free(struct Parser *p)
{
	if (p) {
		for (size_t i = 0; i < BUF_CAP; ++i) {
			free_token(p->buf[i]);
		}
		free_token(p->last_popped);
		close(p->infd);
	}
	free(p);
}

token peek(struct Parser *p)
{
	if (p->buf_use < p->buf_cap) {
		p->buf[p->buf_use++] = read_token(p->infd, NULL);
	}
	return p->buf[p->buf_use - 1];
}

token next(struct Parser *p)
{
	free_token(p->last_popped);
	p->last_popped = NULL;
	if (p->buf_use > 0) {
		p->last_popped = p->buf[p->buf_use - 1];
		p->buf[p->buf_use - 1] = NULL;
		p->buf_use -= 1;
	} else {
		p->last_popped = read_token(p->infd, p->last_popped);
	}
	return p->last_popped;
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
	case TOKEN_EOF:
	case TOKEN_RPAREN:
		return expr;
	case TOKEN_OPERATOR: { /* Dyadic (binop) */
		token t = next(p);
		return make_binop(expr, get_value(t), Expr(p, next(p)));
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
	if (get_type(peek(p)) != TOKEN_NUMBER) { // t is a number.
		return make_number(get_value(t));
	}

	// Otherwise, we have a vector.
	ASTNode vec = make_vector(get_value(t));
	while (get_type(peek(p)) == TOKEN_NUMBER) {
		t = next(p);
		vec = extend_vector(vec, get_value(t));
	}
	return vec;
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
		t = next(p);
		assert(get_type(t) == TOKEN_RPAREN); /* TODO: Error handling. */
		break;
	case TOKEN_NUMBER:
		expr = NumberOrVector(p, t);
		break;
	case TOKEN_OPERATOR:
		expr = make_unop(get_value(t), Expr(p, next(p)));
		break;
	default:
		printf("DEBUG: %s\n", get_value(t));
		assert(0); /* TODO: Error handling */
		return NULL;
	};
	return expr; /* TODO: Indexing. */
}

int parse(int in, FILE *out)
{
	struct Parser *p = parser_make(in);
	assert(p); /* TODO: Error handling */
	ASTNode tree = Expr(p, next(p));
	char *result = value_stringify(Eval(tree));
	fprintf(out, "%s\n", result);
	free(result);
	free_node(tree);
	parser_free(p);
	return 0;
}
