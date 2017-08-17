#include <stdio.h>	          /* FILE*, getc() */
#include <ctype.h>	          /* isdigit(), isalpha() */
#include <stdlib.h>	          /* atoi() */
#include <unistd.h>	          /* write() */
#include <errno.h>	          /* errno */
#include <string.h>	          /* strerror() */
#include "mem/mem.h"		  /* mem_alloc(), mem_dealloc() */
#include "token/token.h"	  /* Tokens for lexer. (struct token) */
#include "string/string.h" /* For lexing tokens (string). */

int lex(FILE* in, int out);
