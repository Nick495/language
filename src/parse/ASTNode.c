
#include "parse.h"

struct ASTNode_ {
	enum { AST_BINOP, AST_UNOP, AST_NUMBER, AST_VECTOR } type;
	union {
		struct { /* Binop */
			ASTNode left;
			char *dyad;
			ASTNode right;
		};
		struct { /* Unop */
			char *monad;
			ASTNode rest;
		};
		Value value; /* Number, vector */
	};
};

/* Uniform allocation (Can make a pool allocator). */
ASTNode make_node(void)
{
	ASTNode n = mem_alloc(sizeof *n);
	assert(n); /* TODO: Error handling */
	return n;
}

/* Returns a char* of the node to string, caller must free. */
char* Stringify(ASTNode n)
{
	char* res = NULL;
	switch(n->type) {
	case AST_BINOP: {
		char* left = Stringify(n->left), *right = Stringify(n->right);
		res = malloc(
			(strlen(left) + strlen(n->dyad) + strlen(right) + 1) * sizeof *res
		);
		sprintf(res, "%s %s %s", left, n->dyad, right);
		free(left);
		free(right);
		break;
	}
	case AST_UNOP: {
		char* rest = Stringify(n->rest);
		res = malloc((strlen(n->monad) + strlen(rest) + 1) * sizeof *res);
		sprintf(res, "%s %s", n->monad, rest);
		free(rest);
		break;
	}
	case AST_NUMBER: /* FALLTHRU */
	case AST_VECTOR:
		res = value_stringify(n->value);
		break;
	default:
		return NULL;
	}
	assert(res); /* TODO: Error handling. */
	return res;
}

Value Eval(ASTNode n)
{
	Value res;
	switch(n->type) {
	case AST_BINOP: {
		Value left = Eval(n->left), right = Eval(n->right);
		res = value_add(left, right);
		value_free(left);
		value_free(right);
		return res;
	}
	case AST_UNOP:
		return Eval(n->rest);
	case AST_NUMBER: /* FALLTHRU */
	case AST_VECTOR:
		return value_reference(n->value);
	}
}

void free_node(ASTNode n)
{
	if (!n) {
		return;
	}
	switch(n->type) {
	case AST_BINOP:
		free_node(n->left);
		free_node(n->right);
		break;
	case AST_NUMBER: /* FALLTHRU */
	case AST_VECTOR:
		value_free(n->value);
		break;
	case AST_UNOP:
		free_node(n->rest);
		break;
	}
	free(n);
}

ASTNode make_binop(ASTNode left, char* dyad, ASTNode right)
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling. */
	n->type = AST_BINOP;
	n->left = left;
	n->dyad = dyad;
	n->right = right;
	return n;
}

ASTNode make_unop(char* monad, ASTNode right)
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling. */
	n->type = AST_UNOP;
	n->monad = monad;
	n->right = right;
	return n;
}

static unsigned long parse_num(char* text)
{
	return strtol(text, NULL, 10);
}

ASTNode make_number(char* val)
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling */
	n->type = AST_NUMBER;
	n->value = value_make_number(parse_num(val));
	return n;
}

ASTNode make_vector(char* val)
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling */
	n->type = AST_VECTOR;
	n->value = value_make_vector(parse_num(val));
	return n;
}

ASTNode extend_vector(ASTNode n, char* val)
{
	assert(n->type == AST_VECTOR);
	n->value = value_append(n->value, parse_num(val));
	return n;
}
