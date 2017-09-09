#include <stdlib.h>         /* exit() */
#include <stdio.h>          /* FILE*, fprintf() */
#include <unistd.h>         /* read() */
#include <string.h>         /* strerror() */
#include <errno.h>          /* errno */
#include "mem/mem.h"		/* mem_alloc(), mem_free() */
#include "token/token.h"	/* Tokens from lexer. (token) */
#include "value/value.h"	/* Value types */
#include "lex/lex.h"		/* lexer. */
#include "ASTNode/ASTNode.h"/* ASTNode definitions */

struct Parser* parser_make();
ASTNode parse(struct Parser *p, char* in, char* in_name);
void parser_free(struct Parser *p);
