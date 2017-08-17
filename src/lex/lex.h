#include <stdio.h>	          /* FILE*, getc() */
#include <ctype.h>	          /* isdigit(), isalpha() */
#include "mem/mem.h"		  /* mem_alloc(), mem_dealloc() */
#include "token/token.h"	  /* Tokens for lexer. (struct token) */

struct lexer;

struct lexer* lexer_make(const char* file_name, FILE* in);
void lexer_free(struct lexer* l);
token lex_token(struct lexer* l, token prev);
