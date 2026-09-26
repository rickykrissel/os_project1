#include "lexer.h"
#include "prompt.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	for (;;) {
		print_prompt();

		char *input = get_input();
		if (input == NULL) {   /* Ctrl+D: end of input */
			printf("\n");
			break;
		}

		tokenlist *tokens = get_tokens(input);

		/* TODO: expansion, builtins and command execution go here */

		free_tokens(tokens);
		free(input);
	}

	return 0;
}