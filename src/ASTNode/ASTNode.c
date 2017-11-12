#include "ASTNode.h"

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
	assert(0); /* Bail on bad type */
	return NULL;
}

/* Returns a char* of the node to string, caller must free. */
struct sizedString Stringify(ASTNode n)
{
	struct sizedString res = {0, NULL};
	switch (n->type) {
	case AST_BINOP: {
		struct sizedString left = Stringify(n->left);
		struct sizedString right = Stringify(n->right);
		struct sizedString dyad = {strlen(n->dyad), n->dyad};
		if (!left.len || !right.len) {
			goto fail_stringify_binop;
		}
		/* Add room for two spaces and a null terminator. */
		res.len = (left.len + dyad.len + right.len + 2 + 1);
		res.text = malloc(res.len * sizeof(*res.text));
		if (!res.text) {
			res.len = 0;
			goto fail_stringify_binop;
		}
		sprintf(res.text, "%s %s %s", left.text, n->dyad, right.text);
	fail_stringify_binop:
		free(left.text);
		free(right.text);
		break;
	}
	case AST_UNOP: {
		struct sizedString monad = {strlen(n->monad), n->monad};
		struct sizedString rest = Stringify(n->rest);
		if (!rest.len) {
			goto fail_stringify_unop;
		}
		/* Add room for a space and a null terminator. */
		res.len = monad.len + rest.len + 2;
		res.text = malloc(res.len * sizeof(*res.text));
		if (!res.text) {
			res.len = 0;
			goto fail_stringify_unop;
		}
		sprintf(res.text, "%s %s", n->monad, rest.text);
	fail_stringify_unop:
		free(rest.text);
		break;
	}
	case AST_VALUE: {
		char *str = value_stringify(n->value);
		res.len = strlen(str);
		res.text = str;
		break;
	}
	case AST_STATEMENT_LIST: {
		size_t i, used_strs = 0, total_len = 0;
		struct sizedString *strs = malloc(sizeof *strs * n->use);
		if (!strs) {
			goto dealloc;
		}
		for (i = 0, used_strs = 0; i < n->use; ++i, ++used_strs) {
			strs[i] = Stringify(n->siblings[i]);
			if (!strs[i].len) {
				goto dealloc;
			}
			total_len += strs[i].len;
		}
		/* Include space for separators "<" + "> " and '\0' */
		res.len = total_len + 3 * n->use + 1;
		res.text = malloc(res.len * sizeof(*res.text));
		if (!res.text) {
			goto dealloc;
		}
		for (i = 0, total_len = 0; i < n->use; ++i) {
			memcpy(res.text + total_len, "<", strlen("<"));
			total_len += strlen("<");
			memcpy(res.text + total_len, strs[i].text, strs[i].len);
			total_len += strs[i].len;
			memcpy(res.text + total_len, "> ", strlen("> "));
			total_len += strlen("> ");
		}
		res.text[total_len + 3 * n->use] = '\0';
	dealloc:
		for (i = 0; i < used_strs; ++i) {
			free(strs[i].text);
		}
		free(strs);
		break;
	}
	case AST_ASSIGNMENT: {
		struct sizedString left = Stringify(n->lvalue);
		struct sizedString right = Stringify(n->rvalue);
		size_t slen = strlen("> := <");
		if (!left.len || !right.len) {
			goto fail_stringify_assignment;
		}
		res.len = 1 + left.len + slen + right.len + 2;
		res.text = malloc(res.len * sizeof(*res.text));
		if (!res.text) {
			res.len = 0;
			goto fail_stringify_assignment;
		}
		sprintf(res.text, "<%s> := <%s>", left.text, right.text);

	fail_stringify_assignment:
		free(left.text);
		free(right.text);
		break;
	}
	default:
		break;
	}
	return res;
}
