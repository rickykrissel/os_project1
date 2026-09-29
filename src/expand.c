
#define _POSIX_C_SOURCE 200809L  
 
#include "expand.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 

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
 
static void expand_env_var(tokenlist *tokens, size_t i)
{
	const char *token = tokens->items[i];
 
	
	if (token[0] != '$' || token[1] == '\0')
		return;
 
	const char *value = getenv(token + 1);   // skip the '$' 
	if (value == NULL)
		value = "";                          
 
	replace_token(tokens, i, value);
}
 

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
 
	const char *rest = token + 1;           
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

 
void expand_tokens(tokenlist *tokens)
{
	if (tokens == NULL)
		return;
 
	for (size_t i = 0; i < tokens->size; i++) {
		expand_env_var(tokens, i);
		expand_tilde(tokens, i);
	}
}
 
