#include "ASTNode.h"
#include "asprintf.c"

/* Returns a char* of the node to string, caller must free. */
char *Stringify(ASTNode n)
{
	char *res = NULL;
	switch (n->type) {
	case AST_ASSIGNMENT: {
		char *left = Stringify(n->lvalue);
		char *right = Stringify(n->rvalue);
		if (!left || !right) {
			goto fail_stringify_assignment;
		}
		asprintf(&res, "<%s> := <%s>", left, right);
	fail_stringify_assignment:
		free(left);
		free(right);
		break;
	}
	case AST_BINOP: {
		char *left = Stringify(n->left), *right = Stringify(n->right);
		if (!left || !right) {
			goto fail_stringify_binop;
		}
		asprintf(&res, "%s %s %s", left, n->dyad, right);
	fail_stringify_binop:
		free(left);
		free(right);
		break;
	}
	case AST_STATEMENT_LIST: {
		char *str;
		size_t i;
		for (i = 0; i < n->use; ++i) {
			str = Stringify(n->siblings[i]);
			if (!str) {
				res = NULL;
				break;
			}
			if (!res) {
				asprintf(&res, "%s", str);
			} else {
				asprintf(&res, "%s <%s> ", res, str);
			}
			free(str);
			if (!res) {
				break;
			}
		}
		break;
	}
	case AST_UNOP: {
		char *rest = Stringify(n->rest);
		if (!rest) {
			goto fail_stringify_unop;
		}
		asprintf(&res, "%s %s", n->monad, res);
	fail_stringify_unop:
		free(rest);
		break;
	}
	case AST_VALUE:
		res = value_stringify(n->value);
		break;
	case AST_TOKEN:
		asprintf(&res, "Token: %s", get_value(&n->tk));
		break;
	case AST_ERROR: /* TODO: Print out real error strings. */
		asprintf(&res, "Error");
		break;
	default:
		assert(0);
		break;
	}
	if (!res) {
		res = strdup("<empty>");
	}
	return res;
}
