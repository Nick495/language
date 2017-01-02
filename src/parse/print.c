#include "print.h"

static void print_token(token t, FILE *out)
{
	char *name;
	switch(get_type(t)) {
	case TOKEN_START:
		name = "start";
		break;
	case TOKEN_EOF:
		name = "eof";
		break;
	case TOKEN_NUMBER:
		name = "number";
		break;
	case TOKEN_OPERATOR:
		name = "operator";
		break;
	case TOKEN_LPAREN:
		name = "Open parenthesis";
		break;
	case TOKEN_RPAREN:
		name = "Close parenthesis";
		break;
	};
	fprintf(out, "Found %s : %s\n", name, get_value(t));
}

int print(int in, FILE *out)
{
	token t = NULL;
	for (t= read_token(in, t); get_type(t) != TOKEN_EOF; t= read_token(in, t)) {
		print_token(t, out);
	}
	free_token(t);
	if (close(in)) {
		fprintf(stderr, "%s\n", strerror(errno));
		return errno;
	}
	return 0;
}
