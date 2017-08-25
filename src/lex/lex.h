#include <stdio.h>	          /* FILE*, getc() */
#include <ctype.h>	          /* isdigit(), isalpha() */
#include "mem/mem.h"		  /* mem_alloc(), mem_dealloc() */
#include "token/token.h"	  /* Tokens for lexer. (struct token) */

struct lexer;

struct lexer* lexer_make();
void lexer_free(struct lexer* l);

token lex_token(struct lexer* l, token prev);
void lexer_init(struct lexer *l, char* in, char* in_name);
