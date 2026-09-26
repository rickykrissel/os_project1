#include "execute.h"
#include "expand.h"
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

		expand_tokens(tokens);

		/* TODO: builtins go here, before external commands */
		if (tokens->size > 0)
			execute_external(tokens);

		free_tokens(tokens);
		free(input);
	}

	return 0;
}