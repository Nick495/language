#include "parse.h"

/* We can guarentee with grammar that the parser never exceeds lookahead. */
#define LOOKAHEAD 2 /* Must be > 1 */
struct Parser {
	struct lexer *lex;	    /* 08 */
	uint32_t buf_read;	    /* 12 Assumes LOOKAHEAD < 2^32 - 1 */
	uint32_t buf_write;	   /* 16 */
	Vm vm;			      /* 24 */
	struct token_ buf[LOOKAHEAD]; /* ((28 or 32) (align)) * LOOKAHEAD  */
	struct symtable_list {	/* 104 */
		struct symtable *symbols; /* 08 */
		struct symtable_list *up; /* 16 */
	} environment;
	struct pool_alloc pool;     /* 140 */
	struct slab_alloc slab;     /* 172 */
	struct ast_node err;	/* 200 */
	struct symtable_list *head; /* 208 */
	char *input_name;	   /* 216 */
};

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
	p->head = &p->environment;
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
	case AST_INBUILT:
	case AST_ERROR:
	case AST_TOKEN: /* Allocated inline. */
		break;
	}
	mem_pool_free(&p->pool, n);
}

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

static int is_empty(struct Parser *p) { return p->buf_write == p->buf_read; }

/* Token type because there's no OOM condition here. */
static token peek(struct Parser *p)
{
	if (is_empty(p)) {
		(void)lex_token(p->lex, &p->buf[p->buf_write]);
		p->buf_write = (p->buf_write + 1) % LOOKAHEAD;
	}
	return &p->buf[p->buf_read];
}

/* Early exit and return out of memory node if found. */
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

static void assign(struct Parser *p, const char *name, struct ast_node n)
{
	symtable_push(p->environment.symbols, &p->slab, name, n);
}

/*
 * Pratt parser, see
 * https://tdop.github.io/
 * https://www.crockford.com/javascript/tdop/tdop.html
 * https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing#id1
*/
typedef ASTNode (*led)(struct Parser *, ASTNode, ASTNode);
typedef ASTNode (*nud)(struct Parser *, ASTNode);
typedef ASTNode (*std)(struct Parser *, ASTNode);
struct token_class {
	led led;
	nud nud;
	std std;
	int lbp;
};

static ASTNode null_nud(struct Parser *p, ASTNode self)
{
	assert(p);
	assert(self);
	assert(self->type == AST_TOKEN);
	printf("Bad unop %s\n", get_value(&self->tk));
	self->type = AST_ERROR;
	self->err = AST_E_INVALID_UNOP;
	return self;
}

static ASTNode null_led(struct Parser *p, ASTNode self, ASTNode left)
{
	assert(p);
	assert(self);
	assert(self->type == AST_TOKEN);
	printf("Bad binop %s\n", get_value(&self->tk));
	self->type = AST_ERROR;
	self->err = AST_E_INVALID_UNOP;
	node_free(p, left);
	return self;
}

/* Forward declare for null_nud, assignment, parens etc. */
static ASTNode expression(struct Parser *p, int rbp);

static ASTNode null_std(struct Parser *p, ASTNode self)
{
	/* Assume expression statement by default. */
	assert(p);
	(void)self;
	return expression(p, 1);
}

static ASTNode identifier_apply(struct Parser *p, ASTNode cur, ASTNode left)
{
	ASTNode res;
	led applied_led;
	nud applied_nud;
	struct symtable_list *look = &p->environment;
	assert(p);
	assert(look != NULL);
	assert(cur);
	assert(cur->type == AST_TOKEN);
	while (look != NULL) {
		res = symtable_find(look->symbols, get_value(&cur->tk),
				    get_len(&cur->tk));
		if (res) {
			break;
		}
		look = look->up;
	}
	if (!res) {
		if (!left) { /* nud */
			return (*null_nud)(p, cur);
		} else { /* led */
			return (*null_led)(p, cur, left);
		}
	}
	assert(res->type == AST_INBUILT);
	if (!res) { /* nud */
		applied_nud = (nud)res->tc.nud;
		return (*applied_nud)(p, cur);
	} else { /* led */
		applied_led = (led)res->tc.led;
		return (*applied_led)(p, cur, left);
	}
}

static struct value_atom parse_atom(ASTNode token)
{
	struct value_atom atom;
	assert(token);
	assert(token->type == AST_TOKEN);
	switch (get_type(&token->tk)) {
	case TOKEN_NUMBER:
		atom.type = INTEGER;
		atom.data.integer = strtoull(get_value(&token->tk), NULL, 0);
		return atom;
	case TOKEN_IDENTIFIER:
		atom.type = SYMBOL;
		atom.data.symbol.text = get_value(&token->tk);
		atom.data.symbol.len = get_len(&token->tk);
		return atom;
	default:
		assert(0); /* Error, bad argument. */
	}
}

static ASTNode SingleOrVec(struct Parser *p, ASTNode n, enum token_type type)
{
	ASTNode spare;
	struct value_atom atom;
	token peeked;
	assert(p);
	assert(n);
	assert(n->type == AST_TOKEN);

	peeked = peek(p);
	atom = parse_atom(n);
	n->type = AST_VALUE;
	if (get_type(peeked) != type) {
		n->value = value_make(p->vm, atom, 1);
		return n;
	}
	n->value = value_make(p->vm, atom, 10);
	while (get_type(peeked) == type) {
		spare = next(p);
		CHECK_OOM(spare);
		atom = parse_atom(spare);
		n->value = value_append(p->vm, n->value, atom);
		peeked = peek(p);
	}
	return n;
}

static ASTNode let_std(struct Parser *p, ASTNode cur)
{
	ASTNode identifiers, rvalues;
	assert(cur);
	assert(p);
	assert(cur->type == AST_TOKEN);
	assert(get_type(&cur->tk) == TOKEN_LET);

	identifiers = SingleOrVec(p, next(p), TOKEN_IDENTIFIER);
	CHECK_OOM(identifiers);
	if (get_type(peek(p)) != TOKEN_ASSIGNMENT) {
		printf("Unexpected token: %d\n", get_type(peek(p)));
		cur->type = AST_ERROR;
		cur->err = AST_E_BAD_ASSIGNMENT;
		return cur;
	}
	(void)next(p); /* Consume the := */
	rvalues = expression(p, 1);
	CHECK_OOM(rvalues);

	cur->type = AST_ASSIGNMENT;
	cur->lvalue = identifiers;
	cur->rvalue = rvalues;
	return cur;
}

static ASTNode paren_nud(struct Parser *p, ASTNode cur)
{
	assert(p);
	assert(cur);
	assert(cur->type == AST_TOKEN);
	cur = expression(p, 2);
	if (get_type(peek(p)) != TOKEN_RPAREN) {
		cur->type = AST_ERROR;
		cur->err = AST_E_BAD_PAREN;
	} else {
		(void)next(p); /* Consume the right paren. */
	}
	return cur;
}

static ASTNode exec_nud(struct Parser *p, ASTNode token)
{
	assert(token);
	assert(token->type == AST_TOKEN);
	switch (get_type(&token->tk)) {
	case TOKEN_NUMBER:
		return SingleOrVec(p, token, TOKEN_NUMBER);
	case TOKEN_IDENTIFIER:
		return identifier_apply(p, token, NULL);
	case TOKEN_LPAREN:
		return paren_nud(p, token);
	}
	return null_nud(p, token);
}

static ASTNode exec_std(struct Parser *p)
{
	token peeked;
	assert(p);
	peeked = peek(p);
	switch (get_type(peeked)) {
	case TOKEN_LET:
		return let_std(p, next(p));
	}
	return null_std(p, NULL);
}

static ASTNode exec_led(struct Parser *p, ASTNode token, ASTNode left)
{
	assert(p);
	assert(token);
	assert(token->type == AST_TOKEN);
	assert(left);
	switch (get_type(&token->tk)) {
	case TOKEN_OPERATOR:
	case TOKEN_IDENTIFIER:
		return identifier_apply(p, token, left);
	}
	return null_led(p, token, left);
}

static int lbp(token token)
{
	assert(token);
	switch (get_type(token)) {
	case TOKEN_SEMICOLON: /* FALLTHRU */
		return 1;
	case TOKEN_EOF:
		return 0;
	case TOKEN_NUMBER:
		return 0xFFFF;
	case TOKEN_RPAREN:
		return 2;
	case TOKEN_OPERATOR: /* '+' */
		return 10;
	default:
		printf("%d\n", get_type(token));
		assert(0); /* Not done yet. */
	}
}

static ASTNode expression(struct Parser *p, int rbp)
{
	ASTNode left, t = next(p);
	token peeked;
	CHECK_OOM(t);
#if 0
	printf("Processing: type: %d | %s\n", get_type(&t->tk),
	       get_value(&t->tk));
#endif
	left = exec_nud(p, t);
	peeked = peek(p);
	while (rbp < lbp(peeked)) {
		t = next(p);
		CHECK_OOM(t);
#if 0
		printf("Processing: type: %d | %s\n", get_type(&t->tk),
		       get_value(&t->tk));
#endif
		left = exec_led(p, t, left);
		peeked = peek(p);
	}
	return left;
}

static ASTNode statement(struct Parser *p)
{
	token peeked;
	ASTNode res;
	assert(p);
	res = exec_std(p);
	CHECK_OOM(res);
	peeked = peek(p);

	if (get_type(peeked) != TOKEN_SEMICOLON) {
		res->type = AST_ERROR;
		res->err = AST_E_BAD_STMT;
	} else {
		(void)next(p); /* Consume semicolon. */
	}
	return res;
}

static ASTNode sl_extend(struct Parser *p, ASTNode sl, size_t cnt,
			 size_t prev_cnt)
{
	assert(sl);
	assert(sl->type == AST_STATEMENT_LIST);

	cnt *= sizeof(*sl->siblings);
	prev_cnt *= sizeof(*sl->siblings);
	sl->siblings = mem_slab_realloc(&p->slab, cnt, sl->siblings, prev_cnt);
	if (!sl->siblings) {
		sl->type = AST_ERROR;
		sl->err = AST_E_NOMEM;
		return sl;
	}
	sl->cap = cnt / sizeof(*sl->siblings);
	return sl;
}

static ASTNode statements(struct Parser *p)
{
	token peeked;
	ASTNode sl = make_node(p, AST_STATEMENT_LIST);
	CHECK_OOM(sl);
	sl_extend(p, sl, 2, 0);
	CHECK_OOM(sl);
	for (;;) {
		if (get_type(peek(p)) == TOKEN_EOF) {
			break;
		}
		sl->siblings[sl->use++] = statement(p);
		if (sl->use == sl->cap) {
			sl_extend(p, sl, sl->cap * 2, sl->cap);
			CHECK_OOM(sl);
		}
	}
	return sl;
}

static ASTNode plus_led(struct Parser *p, ASTNode self, ASTNode left)
{
	ASTNode right;
	assert(self);
	assert(left);
	assert(self->type == AST_TOKEN);
	assert(left->type == AST_VALUE);
	right = expression(p, 2);
	CHECK_OOM(right);
	self->type = AST_BINOP;
	/* TODO: Need to specify what kind of binop. */
	self->left = left;
	self->right = right;
	return self;
}

void add_inbuilt_funcs(struct Parser *p)
{
	assert(p);
	struct ast_node n;
	n.type = AST_INBUILT;

	/* plus */
	n.tc.led = (AST_led)plus_led;
	n.tc.nud = (AST_nud)null_nud; /* No unary op */
	n.tc.std = (AST_std)null_nud; /* No statement op */
	assign(p, "+", n);
}

ASTNode parse(struct Parser *p, char *in, char *in_name)
{
	assert(p);
	assert(in);
	lexer_init(p->lex, in, in_name);
	p->input_name = in_name;
	add_inbuilt_funcs(p);
	return statements(p);
}
