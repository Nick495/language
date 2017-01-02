#include "parse.h"

#define BUF_CAP 1
struct Parser {
	int infd;
	token buf[BUF_CAP];
	token last_popped;
	size_t buf_use;
	size_t buf_cap;
};

struct Parser *make_parser(int in)
{
	struct Parser *p = malloc(sizeof *p);
	assert(p); /* TODO: Error handling. */
	p->infd = in;
	p->buf_use = 0;
	p->buf_cap = BUF_CAP;
	p->last_popped = 0;
	return p;
}

void free_parser(struct Parser *p)
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
	if (p->buf_use) {
		p->last_popped = p->buf[p->buf_use-- - 1];
	} else {
		p->last_popped = read_token(p->infd, p->last_popped);
	}
	return p->last_popped;
}

typedef struct Value_ Value;
struct Value_ {
	enum { VALUE_NUMBER, VALUE_VECTOR } type;
	union {
		long value; /* Number */
	};
};

typedef struct ASTNode_* ASTNode;
struct ASTNode_ {
	enum { AST_BINOP, AST_NUMBER, AST_UNOP } type;
	union {
		struct { /* Binop */
			ASTNode left;
			char *dyad;
			ASTNode right;
		};
		long value; /* Number */
		struct { /* Unop */
			char *monad;
			ASTNode rest;
		};
	};
};

/* Uniform allocation (Can make a pool allocator). */
ASTNode make_node(void)
{
	ASTNode n = malloc(sizeof *n);
	assert(n); /* TODO: Error handling */
	return n;
}

/* Returns a char* of the node to string, caller must free. */
char *stringify(ASTNode n)
{
	switch(n->type) {
	case AST_BINOP: {
		char *res=NULL, *left=stringify(n->left), *right=stringify(n->right);
		asprintf(&res, "%s %s %s", left, n->dyad, right);
		free(left);
		free(right);
		assert(res); /* TODO: Error handling. */
		/* TODO Clean up (factor out) asprintf() free() pattern. */
		return res;
	}
	case AST_NUMBER: {
		char *res = NULL;
		asprintf(&res, "%ld", n->value);
		return res;
	}
	case AST_UNOP: {
		char *res = NULL, *rest = stringify(n->rest);
		asprintf(&res, "%s %s", n->monad, rest);
		free(rest);
		assert(res);
		return res;
	}
	default:
		return NULL;
	}
}

long eval(ASTNode n)
{
	switch(n->type) {
	case AST_BINOP:
		return eval(n->left) + eval(n->right); /* TODO: Ops */
	case AST_NUMBER:
		return n->value;
	case AST_UNOP:
		return eval(n->rest); /* TODO: Ops */
	}
}

void free_node(ASTNode n)
{
	switch(n->type) {
	case AST_BINOP:
		free(n->left);
		free(n->right);
		free(n);
		break;
	case AST_NUMBER:
		break;
	case AST_UNOP:
		free(n->rest);
		free(n);
		break;
	}
}

ASTNode make_binop(ASTNode left, char *dyad, ASTNode right)
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling. */
	n->type = AST_BINOP;
	n->left = left;
	n->dyad = dyad;
	n->right = right;
	return n;
}

ASTNode make_unop(char *monad, ASTNode right)
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling. */
	n->type = AST_UNOP;
	n->monad = monad;
	n->right = right;
	return n;
}

ASTNode make_number(long val)
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling */
	n->type = AST_NUMBER;
	n->value = val;
	return n;
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

ASTNode Number(struct Parser *p, token t)
{
	(void)p; /* Silence unused variable warning. */
	return make_number(strtol(get_value(t), NULL, 10));
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
	case TOKEN_LPAREN: {
		expr = Expr(p, next(p));
		token t = next(p);
		assert(get_type(t) == TOKEN_RPAREN); /* TODO: Error handling. */
		break;
	}
	case TOKEN_NUMBER:
		expr = Number(p, t);
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
	struct Parser *p = make_parser(in);
	assert(p); /* TODO: Error handling */
	ASTNode tree = Expr(p, next(p));
	fprintf(out, "Got %ld\n", eval(tree));
	free_node(tree);
	free_parser(p);
	return 0;
}
