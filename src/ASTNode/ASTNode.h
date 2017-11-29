#ifndef ASTNODE_H_
#define ASTNODE_H_

typedef struct ast_node *ASTNode;

#include "mem/mem.h"     /* Memory management */
#include "token/token.h" /* Token type */
#include "value/value.h" /* Value types, value_stringify() */
#include <assert.h>      /* assert() */
#include <errno.h>       /* errno */
#include <stdio.h>       /* asprintf() */
#include <string.h>      /* strerror() */

enum AST_ERR {
	AST_E_NOMEM,
	AST_E_INVALID_UNOP,
	AST_E_INVALID_BINOP,
	AST_E_BAD_STMT,
	AST_E_BAD_PAREN,
	AST_E_BAD_ASSIGNMENT,
	AST_E_BAD_FUNC
};

typedef int (*AST_led)(int, int, int);
typedef int (*AST_nud)(int, int);
typedef int (*AST_std)(int, int);

struct ast_node { /* 4 + 24 bytes = 28 bytes. */
	enum AST_TYPE {
		AST_BINOP = 0,
		AST_UNOP,
		AST_VALUE,
		AST_ASSIGNMENT,
		AST_STATEMENT_LIST,
		AST_ERROR,
		AST_TOKEN,
		AST_INBUILT,
		AST_FUNC
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
		struct { /* Statement list 24 bytes */
			size_t use;
			size_t cap;
			ASTNode *siblings;
		};
		enum AST_ERR err; /* Error */
		struct token_ tk; /* AST_TOKEN */
		struct {	  /* token class. 24 bytes */
			AST_led led;
			AST_nud nud;
			AST_std std;
		} tc;    /* AST_INBUILT */
		struct { /* 32 */
			ASTNode left;
			ASTNode right;
			ASTNode name;
			ASTNode body;
		} func; /* AST_FUNCTION */
	};
};

/* Returns a char* of the node to string, caller must free. */
char *Stringify(ASTNode n);

#endif
