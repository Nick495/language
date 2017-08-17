#include <errno.h>	        /* errno */
#include <stdio.h>	        /* FILE* */
#include <stdlib.h>         /* atoi() */
#include <string.h>	        /* strerror() */
#include <unistd.h>	        /* read() */

#include "lex/lex.h"
#include "token/token.h"	/* Tokens for lexer. (struct token) */

int print(FILE* in, FILE *out);
