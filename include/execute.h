#ifndef EXECUTE_H
#define EXECUTE_H

#include "lexer.h"
#include "shell.h"


command_result execute_command(tokenlist *tokens, const char *line, shell_state *shell);

#endif
