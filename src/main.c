#include "builtins.h"
#include "execute.h"
#include "expand.h"
#include "lexer.h"
#include "prompt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
	shell_state shell;
	memset(&shell, 0, sizeof shell);
	shell.jobs.next_number = 1;

	setvbuf(stdin, NULL, _IONBF, 0);

	for (;;) {


		jobs_poll(&shell.jobs);
		print_prompt();

		char *input = get_input();
		if (input == NULL) {   
			printf("\n");
			shell_finish(&shell);
			break;
		}

		tokenlist *tokens = get_tokens(input);

		expand_tokens(tokens);

		
		jobs_poll(&shell.jobs);

		command_result result = COMMAND_ERROR;
		if (tokens->size > 0) {
			result = execute_command(tokens, input, &shell);
			if (result == COMMAND_OK)
				history_record(&shell, input);
		}

		free_tokens(tokens);
		free(input);
		if (result == COMMAND_EXIT)
			break;
	}

	history_free(&shell);
	return 0;
}
