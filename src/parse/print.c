#include "print.h"

static void print_token(token t, FILE* out)
{
	char* name;
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

int print(FILE* in, FILE *out)
{
	token t = NULL;
	int c;
	size_t bufuse = 1;
	size_t bufsize = 1024;
	char* buf = malloc(sizeof *buf * bufsize);
	struct lexer* lex = lexer_make();
	assert(buf); /* TODO: Error handling. */
	assert(lex); /* TODO: Error handling. */
	while ((c = getchar_unlocked()) != EOF) {
		buf[bufuse++] = (char) c;
		if (bufuse == bufsize) {
			bufsize *= 2;
			buf = realloc(buf, bufsize);
			assert(buf); /* TODO: Error handling. */
		}
	}
	buf[bufuse + 1] = '\0'; /* Guarenteed to work by mathematical relationship. */
	assert(strlen(buf) == bufuse);

	lexer_init(lex, buf, "stdin");
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
