#include <stdio.h>          /* FILE*, getc() */
#include <unistd.h>         /* fork(), pipe() */
#include "../lex/lex.h"	    /* Lexes tokens. */
#include "../parse/parse.h"	/* Parses tokens. */

int main(void)
{
	int fds[2];
	pipe(fds);
	switch(fork()) {
	case -1: /* error */
		close(fds[1]);
		close(fds[0]);
		return -1;
	case 0: /* child */
		close(fds[1]);
		return parse(fds[0], stdout);
	default: /* parent */
		close(fds[0]);
		return lex(stdin, fds[1]);
	}
	return 0;
}
