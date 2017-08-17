#include <stdio.h>          /* FILE*, getc() */
#include "../parse/parse.h"	/* Parses tokens. */

int main(void)
{
	return parse(stdin, stdout);
}
