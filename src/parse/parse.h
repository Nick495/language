#include "ASTNode/ASTNode.h"   /* ASTNode definitions */
#include "lex/lex.h"	   /* lexer. */
#include "mem/mem.h"	   /* mem_alloc(), mem_free() */
#include "symtable/symtable.h" /* Symbol table */
#include "token/token.h"       /* Tokens from lexer. (token) */
#include "value/value.h"       /* Value types */
#include <errno.h>	     /* errno */
#include <stdio.h>    /* FILE*, fprintf() */
#include <stdlib.h>	    /* exit() */
#include <string.h>	    /* strerror() */
#include <unistd.h>	    /* read() */

struct Parser *parser_make(Vm vm);
ASTNode parse(struct Parser *p, char *in, char *in_name);
void parser_free_ast(struct Parser *p, ASTNode n);
void parser_free(struct Parser *p);
