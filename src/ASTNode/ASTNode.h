#ifndef ASTNODE_H_
#define ASTNODE_H_

#include "mem/mem.h"     /* Memory management */
#include "token/token.h" /* Token type */
#include "value/value.h" /* Value types */
#include <assert.h>      /* assert() */
#include <errno.h>       /* errno */
#include <stdio.h>       /* asprintf() */
#include <string.h>      /* strerror() */

typedef struct ast_node *ASTNode;
enum AST_ERR { AST_E_NOMEM };
struct ast_node { /* 4 + 24 bytes = 28 bytes. */
	enum AST_TYPE {
		AST_BINOP,
		AST_UNOP,
		AST_VALUE,
		AST_ASSIGNMENT,
		AST_STATEMENT_LIST,
		AST_ERROR,
		AST_TOKEN
	} type;		 /* 3 bits necessary. 4 bytes likely. */
	union {		 /* Max 24 bytes */
		struct { /* Binop 24 bytes */
			ASTNode left;
			const char *dyad;
			ASTNode right;
		};
		struct { /* Unop 16 bytes */
			const char *monad;
			ASTNode rest;
		};
		Value value; /* Value 8 bytes */
		struct {     /* Assignment 16 bytes */
			ASTNode lvalue;
			ASTNode rvalue;
		};
		struct { /* Statement list 16 bytes */
			size_t use;
			size_t cap;
			ASTNode *siblings;
		};
		enum AST_ERR err; /* Error */
		struct token_ tk; /* Not used, here for allocation size. */
	};
};

/* Returns a char* of the node to string, caller must free. */
char *Stringify(ASTNode n);
Value Eval(ASTNode n, Vm vm);

#endif
