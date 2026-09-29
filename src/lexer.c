#define _POSIX_C_SOURCE 200809L   /* for strdup() */
 
#include "lexer.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#define CHUNK_SIZE 64
 
/* ---------- small allocation helpers ---------- */
 
static void *xmalloc(size_t n)
{
	void *p = malloc(n);
	if (p == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	return p;
}
 
static void *xrealloc(void *old, size_t n)
{
	void *p = realloc(old, n);
	if (p == NULL) {
		perror("realloc");
		exit(EXIT_FAILURE);
	}
	return p;
}
 
/* ---------- reading input ---------- */
 
char *get_input(void)
{
	size_t cap = CHUNK_SIZE;
	size_t len = 0;
	char *line = xmalloc(cap);
	char chunk[CHUNK_SIZE];
 
	line[0] = '\0';
 
	/* fgets reads at most CHUNK_SIZE - 1 chars at a time, so keep
	 * appending chunks until we see the newline or hit EOF. */
	while (fgets(chunk, sizeof chunk, stdin) != NULL) {
		size_t n = strlen(chunk);
 
		while (len + n + 1 > cap)
			cap *= 2;
		line = xrealloc(line, cap);
 
		memcpy(line + len, chunk, n + 1);
		len += n;
 
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = '\0';   /* strip the newline */
			return line;
		}
	}
 
	/* EOF: return what we have, or NULL if nothing was typed */
	if (len == 0) {
		free(line);
		return NULL;
	}
	return line;
}
 
/* ---------- token list ---------- */
 
static tokenlist *new_tokenlist(void)
{
	tokenlist *tokens = xmalloc(sizeof *tokens);
	tokens->size = 0;
	tokens->items = xmalloc(sizeof(char *));
	tokens->items[0] = NULL;
	return tokens;
}
 
static void add_token(tokenlist *tokens, const char *item)
{
	/* room for the new token plus the terminating NULL */
	tokens->items = xrealloc(tokens->items, (tokens->size + 2) * sizeof(char *));
 
	tokens->items[tokens->size] = strdup(item);
	if (tokens->items[tokens->size] == NULL) {
		perror("strdup");
		exit(EXIT_FAILURE);
	}
	tokens->size++;
	tokens->items[tokens->size] = NULL;
}
 
void free_tokens(tokenlist *tokens)
{
	if (tokens == NULL)
		return;
	for (size_t i = 0; i < tokens->size; i++)
		free(tokens->items[i]);
	free(tokens->items);
	free(tokens);
}
 
/* ---------- tokenizing ---------- */
 
static int is_operator(char c)
{
	return c == '<' || c == '>' || c == '|' || c == '&';
}
 
/*
 * Returns a copy of input with spaces around every operator, so that
 * strtok() will split them off: "ls>out&" becomes "ls > out & ".
 * Each character becomes at most 3, so 3 * len + 1 is always enough.
 */
static char *pad_operators(const char *input)
{
	size_t len = strlen(input);
	char *out = xmalloc(3 * len + 1);
	size_t j = 0;
 
	for (size_t i = 0; i < len; i++) {
		if (is_operator(input[i])) {
			out[j++] = ' ';
			out[j++] = input[i];
			out[j++] = ' ';
		} else {
			out[j++] = input[i];
		}
	}
	out[j] = '\0';
	return out;
}
 
tokenlist *get_tokens(const char *input)
{
	tokenlist *tokens = new_tokenlist();
 
	/* strtok() modifies the string it scans, so work on a copy and
	 * leave the caller's line intact (needed later for the jobs
	 * command line and the exit history). */
	char *buf = pad_operators(input);
 
	/* same loop as tokenization.c, but splitting on tabs too and
	 * saving each token instead of printing it */
	char *tok = strtok(buf, " \t");
	while (tok != NULL) {
		add_token(tokens, tok);
		tok = strtok(NULL, " \t");
	}
 
	free(buf);
	return tokens;
}
 
