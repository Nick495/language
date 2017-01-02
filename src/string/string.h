#ifndef STRING_H_
#define STRING_H_

#include <assert.h> /* assert() */
#include <stdlib.h> /* malloc(), realloc(), free() */
#include <string.h>	/* strlen(), memmove() */

typedef struct string_ *string;

string make_string(void); /* Requires calling free_string() afterward. */
void free_string(string); /* Deallocates string allocated by init_string() */

void empty_string(string); /* Empty the string. */
size_t get_length(string); /* Returns the string's length */
char *get_text(string); /* Gets the text of the string '\0' */
int string_append_char(string, char); /* Append a character to a string. */
int string_append_cstr(string, char*); /* Append a c string to a string. */

#endif
