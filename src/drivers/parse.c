#include <stdio.h>          /* FILE*, getc() */
#include <stdlib.h>			/* malloc(), realloc() */
#include "../parse/parse.h"	/* Parses tokens. */
#include "../ASTNode/ASTNode.h" /* node_free() */

struct darray {
	size_t use;
	size_t cap;
	char *str;
};

struct darray make_darray(void)
{
	struct darray tmp;
	tmp.use = 0;
	tmp.cap = 1024;
	tmp.str = malloc(sizeof *tmp.str * tmp.cap);
	assert(tmp.str); /* TODO: Error handling. */
	return tmp;
}
void darray_push(struct darray *d, char c)
{
	d->str[d->use++] = c;
	if (d->use == d->cap) {
		d->cap *= 2;
		d->str = realloc(d->str, d->cap);
		assert(d->str); /* TODO: Error handling. */
	}
}
void darray_free(struct darray* d)
{
	free(d->str);
}

int main(void)
{
	int c;
	struct Parser* p = parser_make();
	struct darray buffer = make_darray();
	char *res;
	ASTNode tree = NULL;
	Value val = NULL;
	assert(p);

	while ((c = getchar_unlocked()) != EOF) {
		darray_push(&buffer, c);
	}
	darray_push(&buffer, '\0');
	assert(strlen(buffer.str) == buffer.use - 1);

	tree = parse(p, buffer.str, "stdin");
	val = Eval(tree);
	res = value_stringify(val);
	printf("%s\n", res);

	free(res);
	value_free(val);
	free_node(tree);
	darray_free(&buffer);
	parser_free(p);
}
