#include "ASTNode.h"

struct ASTNode_ {
	enum { AST_BINOP,
	       AST_UNOP,
	       AST_VALUE,
	       AST_ASSIGNMENT,
	       AST_STATEMENT_LIST } type;
	union {
		struct { /* Binop */
			ASTNode left;
			const char *dyad;
			ASTNode right;
		};
		struct { /* Unop */
			const char *monad;
			ASTNode rest;
		};
		Value value; /* Value */
		struct {     /* Assignment */
			ASTNode lvalue;
			ASTNode rvalue;
		};
		struct { /* Statement list */
			size_t use;
			size_t cap;
			ASTNode *siblings;
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

void free_node(ASTNode n)
{
	if (!n) {
		return;
	}
	switch (n->type) {
	case AST_BINOP:
		free_node(n->left);
		free_node(n->right);
		break;
	case AST_VALUE:
		value_free(n->value);
		break;
	case AST_UNOP:
		free_node(n->rest);
		break;
	case AST_ASSIGNMENT:
		free_node(n->lvalue);
		free_node(n->rvalue);
		break;
	case AST_STATEMENT_LIST: {
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

ASTNode make_binop(ASTNode left, const char *dyad, ASTNode right)
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling. */
	n->type = AST_BINOP;
	n->left = left;
	n->dyad = dyad;
	n->right = right;
	return n;
}

ASTNode make_unop(const char *monad, ASTNode right)
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling. */
	n->type = AST_UNOP;
	n->monad = monad;
	n->right = right;
	return n;
}

/* TODO: Floating point, other primitive types. */
/* TODO: Should this be in the parser instead? */
static unsigned long parse_num(const char *text)
{
	return strtol(text, NULL, 10);
}

ASTNode make_value(const char *val, enum value_type type, size_t init_size)
{
	ASTNode n = make_node();
	struct value_atom atom = {type, {parse_num(val)}};
	assert(n); /* TODO: Error handling */
	n->type = AST_VALUE;
	n->value = value_make(atom, init_size);
	return n;
}

ASTNode extend_vector(ASTNode vec, const char *val, enum value_type type)
{
	struct value_atom atom = {type, {parse_num(val)}};
	assert(vec->type == AST_VALUE);
	vec->value = value_append(vec->value, atom);
	return vec;
}

ASTNode make_assignment(ASTNode lvalue, ASTNode rvalue)
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling */
	n->type = AST_ASSIGNMENT;
	n->lvalue = lvalue;
	n->rvalue = rvalue;
	return n;
}

ASTNode make_statement_list()
{
	ASTNode n = make_node();
	assert(n); /* TODO: Error handling */
	n->type = AST_STATEMENT_LIST;
	n->use = 0;
	n->cap = 10;
	n->siblings = mem_alloc(sizeof *n->siblings * n->cap);
	assert(n->siblings); /* TODO: Error handling. */
	return n;
}

ASTNode extend_statement_list(ASTNode list, ASTNode stmt)
{
	list->siblings[list->use++] = stmt;
	if (list->use < list->cap) {
		return list;
	}

	ASTNode *new = NULL;
	new = mem_realloc(list->siblings, sizeof *new * list->cap * 2);
	assert(new); /* TODO: Error handling. */
	list->cap *= 2;
	list->siblings = new;
	return list;
}

Value Eval(ASTNode n)
{
	Value res;
	switch (n->type) {
	case AST_BINOP: {
		Value left = Eval(n->left), right = Eval(n->right);
		res = value_add(left, right);
		value_free(left);
		value_free(right);
		return res;
	}
	case AST_UNOP:
		return Eval(n->rest);
	case AST_ASSIGNMENT:
		return Eval(n->rvalue);
	case AST_VALUE:
		return value_reference(n->value);
	case AST_STATEMENT_LIST:
		return Eval(n->siblings[n->use - 1]);
	}
	assert(0);
	return NULL;
}

/* Returns a char* of the node to string, caller must free. */
char *Stringify(ASTNode n)
{
	char *res = NULL;
	switch (n->type) {
	case AST_BINOP: {
		char *left = Stringify(n->left), *right = Stringify(n->right);
		size_t llen = strlen(left), dlen = strlen(n->dyad),
		       rlen = strlen(right);
		/* Add room for two spaces and a null terminator. */
		res = malloc((llen + dlen + rlen + 2 + 1) * sizeof *res);
		assert(res); /* TODO: Error handling. */
		sprintf(res, "%s %s %s", left, n->dyad, right);
		free(left);
		free(right);
		break;
	}
	case AST_UNOP: {
		char *rest = Stringify(n->rest);
		/* Add room for a space and a null terminator. */
		res = malloc((strlen(n->monad) + strlen(rest) + 1 + 1) *
			     sizeof *res);
		assert(res); /* TODO: Error handling. */
		sprintf(res, "%s %s", n->monad, rest);
		free(rest);
		break;
	}
	case AST_VALUE: /* FALLTHRU */
		res = value_stringify(n->value);
		break;
	case AST_STATEMENT_LIST: {
		char **strs =
		    mem_alloc(sizeof *strs * n->use); /* TODO: Error h */
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
	case AST_ASSIGNMENT: {
		char *lv = Stringify(n->lvalue), *rv = Stringify(n->rvalue);
		size_t lvlen = strlen(lv), rvlen = strlen(rv),
		       slen = strlen("> := <");
		res = malloc(1 + lvlen + slen + rvlen + 1 + 1);
		assert(res); /* TODO: Error handling. */
		sprintf(res, "<%s> := <%s>", lv, rv);
		free(lv);
		free(rv);
		break;
	}
	default:
		return NULL;
	}
	assert(res);
	return res;
}
