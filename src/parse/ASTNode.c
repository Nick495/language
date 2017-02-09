
#include "parse.h"

struct ASTNode_ {
	enum { AST_BINOP, AST_NUMBER, AST_UNOP } type;
	union {
		struct { /* Binop */
			ASTNode left;
			char *dyad;
			ASTNode right;
		};
		Value value; /* Number */
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
char *Stringify(ASTNode n)
{
	switch(n->type) {
	case AST_BINOP: {
		char *res=NULL, *left=Stringify(n->left), *right=Stringify(n->right);
		asprintf(&res, "%s %s %s", left, n->dyad, right);
		free(left);
		free(right);
		assert(res); /* TODO: Error handling. */
		/* TODO Clean up (factor out) asprintf() free() pattern. */
		return res;
	}
	case AST_NUMBER: {
		char *res = NULL;
		asprintf(&res, "%s", value_stringify(n->value));
		return res;
	}
	case AST_UNOP: {
		char *res = NULL, *rest = Stringify(n->rest);
		asprintf(&res, "%s %s", n->monad, rest);
		free(rest);
		assert(res);
		return res;
	}
	default:
		return NULL;
	}
}

Value Eval(ASTNode n)
{
	switch(n->type) {
	case AST_BINOP:
		return add_values(Eval(n->left), Eval(n->right)); /* TODO: Ops */
	case AST_NUMBER:
		return n->value;
	case AST_UNOP:
		return Eval(n->rest); /* TODO: Ops */
	}
}

void free_node(ASTNode n)
{
	switch(n->type) {
	case AST_BINOP:
		free(n->left);
		free(n->right);
		break;
	case AST_NUMBER:
		break;
	case AST_UNOP:
		free(n->rest);
		break;
	}
	free(n);
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
	n->value = value_make_number(val);
	return n;
}
