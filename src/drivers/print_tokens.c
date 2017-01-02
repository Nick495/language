#include <stdio.h>          /* FILE*, getc() */
#include <unistd.h>         /* fork(), pipe() */
#include "../lex/lex.h"     /* Lexes tokens. */
#include "../parse/print.h" /* Prints tokens. */

int main(void)
{
	int fds[2];
	pipe(fds); /* Create read, write fd pair. */
	switch(fork()) {
	case -1: /* error */
		close(fds[1]);
		close(fds[0]);
		return -1;
	case 0: /* child */
		close(fds[1]);
		return print(fds[0], stdout);
	default: /* parent */
		close(fds[0]);
		return lex(stdin, fds[1]);
	}
	return 0;
}
