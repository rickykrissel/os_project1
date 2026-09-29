#ifndef BUILTINS_H
#define BUILTINS_H

#include "lexer.h"
#include "shell.h"

int is_builtin(const char *command);
/* In a pipeline/background child, exit must not wait for the parent's jobs. */
command_result execute_builtin(tokenlist *tokens, shell_state *shell, int in_child);
void history_record(shell_state *shell, const char *command);
void history_free(shell_state *shell);
/* Wait for background jobs and print history, used by exit and EOF. */
void shell_finish(shell_state *shell);

#endif
