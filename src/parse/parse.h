#include <stdlib.h>         /* exit() */
#include <stdio.h>          /* FILE*, fprintf() */
#include <unistd.h>         /* read() */
#include <string.h>         /* strerror() */
#include <errno.h>          /* errno */
#include "../token/token.h" /* Tokens from lexer. (token) */
#include "../value/value.h" /* Value types */

int parse(int in, FILE *out);
