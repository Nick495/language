#ifndef ASTNODE_H_
#define ASTNODE_H_

#include "mem/mem.h"     /* mem_alloc(), mem_free() */
#include "value/value.h" /* Value types */
#include <assert.h>      /* assert() */
#include <errno.h>       /* errno */
#include <stdio.h>       /* asprintf() */
#include <string.h>      /* strerror() */

typedef struct ASTNode_ *ASTNode;

/* Returns a char* of the node to string, caller must free. */
char *Stringify(ASTNode n);
Value Eval(ASTNode n);
void free_node(ASTNode n);

ASTNode make_binop(ASTNode left, const char *dyad, ASTNode right);
ASTNode make_unop(const char *monad, ASTNode right);

ASTNode make_single(const char *val, enum value_type type);
ASTNode make_vector(const char *val, enum value_type type);
ASTNode extend_vector(ASTNode vec, const char *val, enum value_type type);
ASTNode make_statement(ASTNode e);
ASTNode make_assignment(ASTNode lvalue, ASTNode rvalue);
ASTNode make_statement_list();
ASTNode extend_statement_list(ASTNode list, ASTNode stmt);
#endif
