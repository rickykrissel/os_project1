#ifndef EXECUTE_H
#define EXECUTE_H

#include "lexer.h"   /* for tokenlist */

/*
 * Runs an external command in the foreground (Part 5).
 *
 * tokens->items[0] is the command name; the whole NULL-terminated
 * items array is passed to execv() as the argument list.
 *
 * Returns 0 if the command was found and run (a "valid" command for
 * the exit history), or -1 if it was not found or could not be started.
 */
int execute_external(tokenlist *tokens);

#endif