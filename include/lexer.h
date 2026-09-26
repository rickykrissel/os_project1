#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

/*
 * A list of tokens produced from one line of input.
 * items is always NULL-terminated, so a single command's tokens
 * can be passed straight to execv().
 */
typedef struct {
	char **items;   /* token strings, followed by a NULL entry */
	size_t size;    /* number of tokens, not counting the NULL */
} tokenlist;

/*
 * Reads one full line from stdin, of any length, with the trailing
 * newline removed. Returns a malloc'd string the caller must free,
 * or NULL on end of input (Ctrl+D).
 */
char *get_input(void);

/*
 * Splits a line into tokens on spaces and tabs. The operators
 * <, >, | and & always become separate tokens, even when written
 * without spaces (e.g. "ls>out" -> "ls", ">", "out").
 * The input string is not modified.
 */
tokenlist *get_tokens(const char *input);

/* Frees a tokenlist and every token in it. */
void free_tokens(tokenlist *tokens);

#endif