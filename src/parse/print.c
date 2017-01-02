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

static enum token_type print_token_wrap(int in, FILE *out)
{
	token t = read_token(in, NULL);
	enum token_type type = get_type(t);
	print_token(t, out);
	free(t);
	return type;
}

int print(int in, FILE *out)
{
	for (enum token_type type = print_token_wrap(in, out); type != TOKEN_EOF;) {
		type = print_token_wrap(in, out);
	}
	if (close(in)) {
		fprintf(stderr, "%s\n", strerror(errno));
		return errno;
	}
	return 0;
}
