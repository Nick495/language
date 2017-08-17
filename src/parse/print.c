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

int print(FILE *in, FILE *out)
{
	token t = NULL;
	struct lexer *lex = lexer_make("stdin", in); 
	assert(lex); /* TODO: Error handling. */

	for (t =lex_token(lex, t); get_type(t) != TOKEN_EOF; t =lex_token(lex, t)) {
		print_token(t, out);
	}
	token_free(t);
	if (fclose(in)) {
		fprintf(stderr, "error in: %s\n", strerror(errno));
		fclose(out);
		return errno;
	}
	if (fclose(out)) {
		fprintf(stderr, "error out: %s\n", strerror(errno));
		return errno;
	}
	lexer_free(lex);
	return 0;
}
