#include "parse.h"

/* We can guarentee with grammar that the parser never exceeds lookahead. */
#define LOOKAHEAD 2 /* Must be > 1 */
struct Parser {
	size_t buf_read;
	size_t buf_write;
	struct lexer *lex;
	token buf[LOOKAHEAD];
	struct symtable_list {
		struct symtable *symbols;
		struct symtable_list *up;
	} environment;
	char *input_name;
};

struct Parser *parser_make()
{
	struct Parser *p = mem_alloc(sizeof *p + token_size() * LOOKAHEAD);
	assert(p); /* TODO: Error handling. */
	p->lex = lexer_make();
	assert(p->lex); /* TODO: Error handling. */
	p->environment.symbols = symtable_make();
	p->environment.up = NULL;
	assert(p->environment.symbols);
	p->buf_read = 0;
	p->buf_write = 0;
	for (size_t i = 0; i < LOOKAHEAD; ++i) {
		p->buf[i] = (token)((char *)p + sizeof *p + token_size() * i);
	}
	return p;
}

void parser_free(struct Parser *p)
{
	if (p) {
		lexer_free(p->lex);
		while (p->environment.up) {
			symtable_free(p->environment.symbols);
			p->environment = *p->environment.up;
		}
	}
	free(p);
}

static int is_empty(struct Parser *p) { return p->buf_write == p->buf_read; }

static token token_pop(struct Parser *p)
{
	token t = NULL;
	t = token_copy(t, p->buf[p->buf_read]);
	p->buf_read = (p->buf_read + 1) % LOOKAHEAD;
	return t;
}

static void token_push(struct Parser *p)
{
	p->buf[p->buf_write] = lex_token(p->lex, p->buf[p->buf_write]);
	p->buf_write = (p->buf_write + 1) % LOOKAHEAD;
}

static token peek(struct Parser *p)
{
	if (is_empty(p)) {
		token_push(p);
	}
	return p->buf[p->buf_read];
}

static token next(struct Parser *p)
{
	token t = NULL;
	if (is_empty(p)) {
		t = lex_token(p->lex, t);
	} else {
		t = token_pop(p);
	}
	assert(t);
	return t;
}

static void assign(struct Parser *p, const char *name, Value value)
{
	symtable_push(p->environment.symbols, name, value);
}

static enum type token_to_value_type(enum token_type token_type)
{
	switch (token_type) {
	case TOKEN_NUMBER:
		return INTEGER;
	case TOKEN_IDENTIFIER:
		return SYMBOL;
	default:
		fprintf(stderr, "Expected type invalid.\n");
		assert(0); /* TODO: Error handling. */
		exit(EXIT_FAILURE);
	}
}

ASTNode Expr(struct Parser *, token);
ASTNode Op(struct Parser *, token);

//	expr
//		operand
//		operand binop expr
ASTNode Expr(struct Parser *p, token t)
{
	ASTNode expr = Op(p, t);
	switch (get_type(peek(p))) {
	case TOKEN_OPERATOR: { /* Dyadic (binop) */
		ASTNode res;
		token t = next(p);
		res = make_binop(expr, get_value(t), Expr(p, next(p)));
		token_free(t);
		return res;
	}
	default:
		return expr;
	}
}

/* Note, can seperate into two functions. */
ASTNode SingleOrVector(struct Parser *p, token t, enum token_type expected_type)
{
	ASTNode res;
	enum type value_type = token_to_value_type(expected_type);
	if (get_type(peek(p)) !=
	    expected_type) { /* No followup, so a single. */
		res = make_single(get_value(t), value_type);
		token_free(t);
		return res;
	}
	res = make_vector(get_value(t), value_type);
	token_free(t);
	while (get_type(peek(p)) == expected_type) {
		t = next(p);
		res = extend_vector(res, get_value(t), value_type);
		token_free(t);
	}
	return res;
}

// Grammar from Rob Pike's talk
//	operand
//		( Expr )
//		( Expr ) [ Expr ]... TODO
//		operand
//		number
//		unop Expr
ASTNode Op(struct Parser *p, token t)
{
	ASTNode op;
	switch (get_type(t)) {
	case TOKEN_LPAREN:
		op = Expr(p, next(p));
		token_free(t);
		t = next(p);
		assert(get_type(t) == TOKEN_RPAREN); /* TODO: Error handling. */
		token_free(t);
		break;
	case TOKEN_NUMBER:
		op = SingleOrVector(p, t, TOKEN_NUMBER);
		break;
	case TOKEN_OPERATOR:
		op = make_unop(get_value(t), Expr(p, next(p)));
		token_free(t);
		break;
	case TOKEN_EOF:
		printf("Extraneous ';'\n");
		token_free(t);
		assert(0);
		return NULL;
	default:
		printf("Unexpected token: %s\n", get_value(t));
		token_free(t);
		assert(0); /* TODO: Error handling */
		return NULL;
	};
	return op;
}

//	statement
//		Expr ;
//		let Expr := Expr ; TODO
ASTNode Statement(struct Parser *p, token t)
{
	ASTNode res;
	switch (get_type(t)) {
	case TOKEN_LET: {
		ASTNode lvalue;
		lvalue = SingleOrVector(p, next(p), TOKEN_IDENTIFIER);
		if (get_type(peek(p)) != TOKEN_ASSIGNMENT) {
			fprintf(stderr, "Unexpected token %s\n",
				get_value(peek(p)));
			assert(0);
		}
		next(p); /* Consume := */
		res = make_assignment(lvalue, Expr(p, next(p)));
		break;
	}
	default: /* By default, an expression statement. */
		res = Expr(p, t);
		break;
	}
	if (get_type(peek(p)) != TOKEN_SEMICOLON) {
		fprintf(stderr, "Unexpected token: %s\n", get_value(peek(p)));
		assert(0);
	}
	next(p); /* Consume the semicolon. */
	return res;
}

//	statementList
//		statement...
ASTNode StatementList(struct Parser *p, token t)
{
	ASTNode res = make_statement_list(), stmt;
	while (get_type(t) != TOKEN_EOF) {
		stmt = Statement(p, t);
		res = extend_statement_list(res, stmt);
		t = next(p);
	}
	return res;
}

ASTNode parse(struct Parser *p, char *in, char *in_name)
{
	assert(p);
	assert(in);
	lexer_init(p->lex, in, in_name);
	p->input_name = in_name;
	return StatementList(p, next(p));
}
