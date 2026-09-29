/*
 * expand.c
 *
 * Part 2: Environment variable expansion  ($USER -> mnguyen)
 * Part 3: Tilde expansion                 (~, ~/dir -> /home/.../dir)
 *
 * Runs after get_tokens() and before any command (built-in or external)
 * looks at the tokens. Tokens are rewritten in place.
 */
 
#define _POSIX_C_SOURCE 200809L   /* for strdup() */
 
#include "expand.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
/* ------------------------------------------------------------------ */
/* Helper: replace one token                                           */
/* ------------------------------------------------------------------ */
 
/* Frees the old token at tokens->items[i] and stores a copy of new_value. */
static void replace_token(tokenlist *tokens, size_t i, const char *new_value)
{
	char *copy = strdup(new_value);
	if (copy == NULL) {
		perror("strdup");
		exit(EXIT_FAILURE);
	}
	free(tokens->items[i]);
	tokens->items[i] = copy;
}
 
/* ------------------------------------------------------------------ */
/* Part 2: environment variables                                       */
/* ------------------------------------------------------------------ */
 
/* If tokens->items[i] starts with '$', replaces it with the variable's value. */
static void expand_env_var(tokenlist *tokens, size_t i)
{
	const char *token = tokens->items[i];
 
	/* not a variable, or a lone "$" (left as-is, like bash) */
	if (token[0] != '$' || token[1] == '\0')
		return;
 
	const char *value = getenv(token + 1);   /* skip the '$' */
	if (value == NULL)
		value = "";                          /* unset -> empty, like bash */
 
	replace_token(tokens, i, value);
}
 
/* ------------------------------------------------------------------ */
/* Part 3: tilde                                                       */
/* ------------------------------------------------------------------ */
 
/*
 * If tokens->items[i] is "~" or starts with "~/", replaces the ~ with $HOME.
 * Anything else (e.g. "~user", "a~b") is left alone.
 */
static void expand_tilde(tokenlist *tokens, size_t i)
{
	const char *token = tokens->items[i];
 
	if (token[0] != '~' || (token[1] != '\0' && token[1] != '/'))
		return;
 
	const char *home = getenv("HOME");
	if (home == NULL) {
		fprintf(stderr, "expand: HOME is not set\n");
		return;
	}
 
	const char *rest = token + 1;            /* "" or "/..." */
	char *expanded = malloc(strlen(home) + strlen(rest) + 1);
	if (expanded == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	strcpy(expanded, home);
	strcat(expanded, rest);
 
	replace_token(tokens, i, expanded);
	free(expanded);
}
 
/* ------------------------------------------------------------------ */
/* Public entry point (declared in expand.h)                           */
/* ------------------------------------------------------------------ */
 
void expand_tokens(tokenlist *tokens)
{
	if (tokens == NULL)
		return;
 
	/* A token can only match one of the two, so order doesn't matter. */
	for (size_t i = 0; i < tokens->size; i++) {
		expand_env_var(tokens, i);
		expand_tilde(tokens, i);
	}
}
 
