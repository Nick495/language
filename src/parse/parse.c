#include "parse.h"

/* We can guarentee with grammar that the parser never exceeds lookahead. */
#define LOOKAHEAD 2 /* Must be > 1 */
struct Parser {
	struct symtable_list {		  /* 16 */
		struct symtable *symbols; /* 08 */
		struct symtable_list *up; /* 16 */
	} environment;
	struct lexer *lex;	    /* 24 */
	uint32_t buf_read;	    /* 28 Assumes LOOKAHEAD < 2^32 - 1 */
	uint32_t buf_write;	   /* 32 */
	Vm vm;			      /* 40 */
	struct token_ buf[LOOKAHEAD]; /* ((28 or 32) (align)) * LOOKAHEAD  */
	struct pool_alloc pool;       /* 88 */
	struct slab_alloc slab;       /* 120 */
	struct ast_node err;	  /* 148 */
	char *input_name;	     /* 156 */
};

/* Uniform allocation (Can make a pool allocator). */
static ASTNode make_node(struct Parser *p, enum AST_TYPE type)
{
	assert(p);
	ASTNode n = mem_pool_alloc(&p->pool);
	if (!n) {
		n = &p->err;
		return n;
	}
	n->type = type;
	return n;
}

static void node_free(struct Parser *p, ASTNode n)
{
	mem_pool_free(&p->pool, n);
}

/* TODO: Refactor to take in a pointer and return an int status code. */
struct Parser *parser_make(Vm vm)
{
	struct Parser *p = malloc(sizeof *p);
	assert(p); /* TODO: Error handling. */
	assert(vm);
	p->buf_read = p->buf_write = 0;
	p->lex = lexer_make();
	assert(p->lex); /* TODO: Error handling. */
	assert(init_slab_alloc(&p->slab, 8 * 4096) == 0);
	assert(init_pool_alloc(&p->pool, &p->slab, sizeof(p->err)) == 0);
	p->vm = vm;
	p->environment.symbols = symtable_make(&p->slab);
	p->environment.up = NULL;
	assert(p->environment.symbols);
	p->err.type = AST_ERROR; /* Preallocate Out of Memory error */
	p->err.err = AST_E_NOMEM;
	return p;
}

void parser_free(struct Parser *p)
{
	if (p) {
		lexer_free(p->lex);
		while (p->environment.up) {
			symtable_free(p->environment.symbols, &p->slab);
			p->environment = *p->environment.up;
		}
		symtable_free(p->environment.symbols, &p->slab);
		mem_pool_deinit(&p->pool);
		mem_slab_deinit(&p->slab);
	}
	free(p);
}

void parser_free_ast(struct Parser *p, ASTNode n)
{
	assert(n != NULL);
	switch (n->type) {
	case AST_BINOP:
		parser_free_ast(p, n->left);
		parser_free_ast(p, n->right);
		break;
	case AST_VALUE:
		value_free(p->vm, n->value);
		break;
	case AST_UNOP:
		parser_free_ast(p, n->rest);
		break;
	case AST_ASSIGNMENT:
		parser_free_ast(p, n->lvalue);
		parser_free_ast(p, n->rvalue);
		break;
	case AST_STATEMENT_LIST: {
		size_t i = 0;
		for (i = 0; i < n->use; ++i) {
			parser_free_ast(p, n->siblings[i]);
		}
		mem_slab_free(&p->slab, n->siblings);
		break;
	}
	case AST_ERROR:
	case AST_TOKEN: /* Allocated inline. */
		break;
	}
	mem_pool_free(&p->pool, n);
}

static int is_empty(struct Parser *p) { return p->buf_write == p->buf_read; }

static token peek(struct Parser *p)
{
	if (is_empty(p)) {
		(void)lex_token(p->lex, &p->buf[p->buf_write]);
		p->buf_write = (p->buf_write + 1) % LOOKAHEAD;
	}
	return &p->buf[p->buf_read];
}

#define CHECK_OOM(t)                                                           \
	do {                                                                   \
		if ((t)->type == AST_ERROR) {                                  \
			return t;                                              \
		}                                                              \
	} while (0)

static ASTNode next(struct Parser *p)
{

	ASTNode n = make_node(p, AST_TOKEN);
	CHECK_OOM(n);
	if (is_empty(p)) {
		lex_token(p->lex, &n->tk);
	} else {
		token_copy(&n->tk, &p->buf[p->buf_read]);
		p->buf_read = (p->buf_read + 1) % LOOKAHEAD;
	}
	return n;
}

static void assign(struct Parser *p, const char *name, Value value)
{
	symtable_push(p->environment.symbols, &p->slab, name, value);
}

static enum value_type token_to_value_type(enum token_type token_type)
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

static ASTNode Expr(struct Parser *, ASTNode);
static ASTNode Op(struct Parser *, ASTNode);

static ASTNode make_binop(struct Parser *p, ASTNode left, const char *dyad,
			  ASTNode right)
{
	ASTNode n = make_node(p, AST_BINOP);
	CHECK_OOM(n);
	n->left = left;
	n->dyad = dyad;
	n->right = right;
	return n;
}

/*
 *	expr
 *		operand
 *		operand binop expr
*/
static ASTNode Expr(struct Parser *p, ASTNode t)
{
	CHECK_OOM(t);
	ASTNode expr = Op(p, t);
	CHECK_OOM(expr);
	switch (get_type(peek(p))) {
	case TOKEN_OPERATOR: { /* Dyadic (binop) */
		ASTNode res, t = next(p);
		CHECK_OOM(t);
		res = make_binop(p, expr, get_value(&t->tk), Expr(p, next(p)));
		return res;
	}
	default:
		return expr;
	}
}

/* TODO: Floating point, other primitive types. */
static unsigned long parse_num(const char *text)
{
	return strtol(text, NULL, 10);
}

static ASTNode make_value(struct Parser *p, const char *val,
			  enum value_type type, size_t init_size)
{
	ASTNode n = make_node(p, AST_VALUE);
	CHECK_OOM(n);
	struct value_atom atom = {type, {parse_num(val)}};
	n->value = value_make(p->vm, atom, init_size);
	return n;
}

static ASTNode extend_vector(struct Parser *p, ASTNode vec, const char *val,
			     enum value_type type)
{
	struct value_atom atom = {type, {parse_num(val)}};
	assert(vec->type == AST_VALUE);
	vec->value = value_append(p->vm, vec->value, atom);
/* TODO: Need a get value method */
#if 0
	if (vec->value->type == ERROR) {
		vec->type = AST_ERROR;
		vec->err = AST_E_NOMEM;
	}
#endif
	return vec;
}

static ASTNode SingleOrVector(struct Parser *p, ASTNode t,
			      enum token_type expected_type)
{
	ASTNode res;
	CHECK_OOM(t);
	enum value_type value_type = token_to_value_type(expected_type);
	if (get_type(peek(p)) != expected_type) {
		/* No followup, so it's a single. */
		res = make_value(p, get_value(&t->tk), value_type, 1);
		return res;
	}
	res = make_value(p, get_value(&t->tk), value_type, 10);
	CHECK_OOM(res);
	while (get_type(peek(p)) == expected_type) {
		t = next(p);
		CHECK_OOM(t);
		res = extend_vector(p, res, get_value(&t->tk), value_type);
		CHECK_OOM(res);
		// node_free(p, t);
	}
	return res;
}

static ASTNode make_unop(struct Parser *p, const char *monad, ASTNode right)
{
	ASTNode n = make_node(p, AST_UNOP);
	CHECK_OOM(n);
	n->monad = monad;
	n->right = right;
	return n;
}

/*
 * Grammar from Rob Pike's talk
 *	operand
 *		( Expr )
 *		( Expr ) [ Expr ]... TODO
 *		operand
 *		number
 *		unop Expr
*/
static ASTNode Op(struct Parser *p, ASTNode t)
{
	ASTNode op;
	CHECK_OOM(t);
	switch (get_type(&t->tk)) {
	case TOKEN_LPAREN:
		op = Expr(p, next(p));
		// node_free(p, t);
		t = next(p);
		CHECK_OOM(t);
		assert(get_type(&t->tk) == TOKEN_RPAREN);
		/* TODO: Error handling. */
		// node_free(p, t);
		break;
	case TOKEN_NUMBER:
		op = SingleOrVector(p, t, TOKEN_NUMBER);
		break;
	case TOKEN_OPERATOR:
		op = make_unop(p, get_value(&t->tk), Expr(p, next(p)));
		break;
	case TOKEN_EOF:
		printf("Extraneous ';'\n");
		// node_free(p, t);
		assert(0);
		return NULL;
	default:
		printf("Unexpected token: %s\n", get_value(&t->tk));
		// node_free(p, t);
		assert(0); /* TODO: Error handling */
		return NULL;
	};
	return op;
}

static ASTNode make_assignment(struct Parser *p, ASTNode lvalue, ASTNode rvalue)
{
	ASTNode n = make_node(p, AST_ASSIGNMENT);
	CHECK_OOM(n);
	n->lvalue = lvalue;
	n->rvalue = rvalue;
	return n;
}

/*
 *	statement
 *		Expr ;
 *		let Expr := Expr ; TODO
*/
static ASTNode Statement(struct Parser *p, ASTNode t)
{
	ASTNode res;
	CHECK_OOM(t);
	switch (get_type(&t->tk)) {
	case TOKEN_LET: {
		ASTNode lvalue;
		lvalue = SingleOrVector(p, next(p), TOKEN_IDENTIFIER);
		CHECK_OOM(lvalue);
		if (get_type(peek(p)) != TOKEN_ASSIGNMENT) {
			fprintf(stderr, "Unexpected token %s\n",
				get_value(peek(p)));
			assert(0);
		}
		CHECK_OOM(next(p)); /* Consume := */
		res = make_assignment(p, lvalue, Expr(p, next(p)));
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
	CHECK_OOM(next(p)); /* Consume the semicolon. */
	return res;
}

static ASTNode make_statement_list(struct Parser *p)
{
	ASTNode n = make_node(p, AST_STATEMENT_LIST);
	CHECK_OOM(n);
	n->use = 0;
	n->cap = 10;
	n->siblings = mem_slab_alloc(&p->slab, sizeof *n->siblings * n->cap);
	assert(n->siblings); /* TODO: Error handling. */
	return n;
}

static ASTNode extend_statement_list(struct Parser *p, ASTNode list,
				     ASTNode stmt)
{
	list->siblings[list->use++] = stmt;
	if (list->use < list->cap) {
		return list;
	}

	ASTNode *new = NULL;
	new = mem_slab_realloc(&p->slab, sizeof(*new) * list->cap * 2,
			       list->siblings, sizeof(*new) * list->cap);
	assert(new); /* TODO: Error handling. */
	list->cap *= 2;
	list->siblings = new;
	return list;
}

/*
 *	statementList
 *		statement...
*/
static ASTNode StatementList(struct Parser *p, ASTNode t)
{
	ASTNode res = make_statement_list(p), stmt;
	CHECK_OOM(res);
	CHECK_OOM(t);
	while (get_type(&t->tk) != TOKEN_EOF) {
		stmt = Statement(p, t);
		res = extend_statement_list(p, res, stmt);
		t = next(p);
		CHECK_OOM(t);
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
