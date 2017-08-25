#include <stdio.h>          /* FILE*, getc() */
#include <stdlib.h>			/* malloc(), realloc() */
#include "../parse/parse.h"	/* Parses tokens. */
#include "../parse/ASTNode.h" /* node_free() */

int main(void)
{
	int c;
	size_t bufuse = 1;
	size_t bufsize = 1024;
	char* buf = malloc(sizeof *buf * bufsize);
	struct Parser* p = parser_make();
	ASTNode tree;
	Value val;
	assert(buf); /* TODO: Error handling. */
	while ((c = getchar_unlocked()) != EOF) {
		buf[bufuse++ - 1] = (char) c;
		if (bufuse == bufsize) {
			bufsize *= 2;
			buf = realloc(buf, bufsize);
			assert(buf); /* TODO: Error handling. */
		}
	}
	buf[bufuse] = '\0';
	assert(strlen(buf) == bufuse - 1);

	tree = parse(p, buf, "stdin");
	val = Eval(tree);
	free_node(tree);
	printf("%s\n", value_stringify(val));
	value_free(val);
	free(buf);
}
