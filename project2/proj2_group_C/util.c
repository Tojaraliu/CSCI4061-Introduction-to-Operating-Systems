#include <stdio.h>
#include <string.h>
#include "util.h"

/*
 * Print shell prompt. Pass the username as argument.
 */
void print_prompt(char *name)
{
		printf("%s >> ", name);
		fflush(stdout);
}

/*
 * Checks if the first string starts with the second. Return 1 if true.
 */
int starts_with(const char *a, const char *b)
{
	if (strncmp(a, b, strlen(b)) == 0)
		return 1;
	else
		return 0;
}
