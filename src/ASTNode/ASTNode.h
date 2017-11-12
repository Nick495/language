#ifndef ASTNODE_H_
#define ASTNODE_H_

#include "mem/mem.h"     /* mem_alloc(), mem_free() */
#include "value/value.h" /* Value types */
#include <assert.h>      /* assert() */
#include <errno.h>       /* errno */
#include <stdio.h>       /* asprintf() */
#include <string.h>      /* strerror() */

typedef struct ASTNode_ *ASTNode;
struct ASTNode_ {
	enum { AST_BINOP,
	       AST_UNOP,
	       AST_VALUE,
	       AST_ASSIGNMENT,
	       AST_STATEMENT_LIST } type;
	union {
		struct { /* Binop */
			ASTNode left;
			const char *dyad;
			ASTNode right;
		};
		struct { /* Unop */
			const char *monad;
			ASTNode rest;
		};
		Value value; /* Value */
		struct {     /* Assignment */
			ASTNode lvalue;
			ASTNode rvalue;
		};
		struct { /* Statement list */
			size_t use;
			size_t cap;
			ASTNode *siblings;
		};
	};
};

struct sizedString {
	size_t len;
	char *text;
};

/* Returns a char* of the node to string, caller must free. */
struct sizedString Stringify(ASTNode n);
Value Eval(ASTNode n);
void free_node(ASTNode n);

#endif
