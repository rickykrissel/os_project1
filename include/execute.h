#ifndef EXECUTE_H
#define EXECUTE_H

#include "lexer.h"
#include "shell.h"

/*
 * Executes up to three pipeline stages, optionally followed by &.
 * Standalone foreground built-ins run in the shell process.
 * COMMAND_OK means the command started successfully, even if an external
 * program later returns nonzero. Syntax, lookup, and startup errors are invalid.
 * tokens and the original command line remain owned by the caller.
 */
command_result execute_command(tokenlist *tokens, const char *line, shell_state *shell);

#endif
