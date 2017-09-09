#include "ASTNode.h"

struct ASTNode_ {
	enum { AST_BINOP, AST_UNOP, AST_NUMBER, AST_VECTOR, AST_STATEMENT } type;
	union {
		struct { /* Binop */
			ASTNode left;
			char* dyad;
			ASTNode right;
		};
		struct { /* Unop */
			char* monad;
			ASTNode rest;
		};
		Value value; /* Number, vector */
		struct { /* statement */
			size_t use;
			size_t cap;
			ASTNode* siblings;
		};
	};
};

/* Uniform allocation (Can make a pool allocator). */
static ASTNode make_node(void)
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
		size_t llen =strlen(left), dlen =strlen(n->dyad), rlen =strlen(right);
		/* Add room for two spaces and a null terminator. */
		res = malloc((llen + dlen + rlen + 2 + 1) * sizeof *res);
		assert(res); /* TODO: Error handling. */
		sprintf(res, "%s %s %s", left, n->dyad, right);
		free(left);
		free(right);
		break;
	}
	case AST_UNOP: {
		char* rest = Stringify(n->rest);
		/* Add room for a space and a null terminator. */
		res = malloc((strlen(n->monad) + strlen(rest) + 1 + 1) * sizeof *res);
		assert(res); /* TODO: Error handling. */
		sprintf(res, "%s %s", n->monad, rest);
		free(rest);
		break;
	}
	case AST_NUMBER: /* FALLTHRU */
	case AST_VECTOR:
		res = value_stringify(n->value);
		break;
	case AST_STATEMENT: {
		char** strs = mem_alloc(sizeof *strs * n->use); /* TODO: Error h */
		size_t i, total_len = 0;
		assert(strs);
		for (i = 0; i < n->use; ++i) {
			strs[i] = Stringify(n->siblings[i]);
			total_len += strlen(strs[i]);
		}
		/* Include space for separators "<" + "> " and '\0' */
		res = malloc(sizeof *res * (total_len + 3 * n->use + 1));
		assert(res); /* TODO: Error handling. */
		total_len = 0;
		for (i = 0; i < n->use; ++i) {
			memcpy(res + total_len, "<", strlen("<"));
			total_len += strlen("<");
			memcpy(res + total_len, strs[i], strlen(strs[i]));
			total_len += strlen(strs[i]);
			memcpy(res + total_len, "> ", strlen("> "));
			total_len += strlen("> ");
		}
		res[total_len + 3 * n->use] = '\0';
		for (i = 0; i < n->use; ++i) {
			mem_dealloc(strs[i]);
		}
		mem_dealloc(strs);
		break;
	}
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
	case AST_STATEMENT:
		return Eval(n->siblings[n->use - 1]);
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
	case AST_STATEMENT: {
		size_t i = 0;
		for (i = 0; i < n->use; ++i) {
			free_node(n->siblings[i]);
		}
		mem_dealloc(n->siblings);
		break;
	}
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

/* TODO: Floating point, other primitive types. */
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
	/* TODO: Allow for other types. */
	n->value = value_append(n->value, parse_num(val));
	return n;
}

ASTNode make_statement(ASTNode e)
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling */
	n->type = AST_STATEMENT;
	n->use = 0;
	n->cap = 10;
	n->siblings = mem_alloc(sizeof *n->siblings * n->cap);
	assert(n->siblings); /* TODO: Error handling. */
	n->siblings[n->use++] = e;
	return n;
}

ASTNode extend_statement(ASTNode s, ASTNode e)
{
	s->siblings[s->use++] = e;
	if (s->use == s->cap) {
		ASTNode* new = NULL;
		new = mem_realloc(s->siblings, sizeof *new * s->cap * 2);
		assert(new); /* TODO: Error handling. */
		mem_dealloc(s->siblings);
		s->cap *= 2;
		s->siblings = new;
	}
	return s;
}
