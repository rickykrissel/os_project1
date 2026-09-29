#ifndef REDIRECT_H
#define REDIRECT_H

#include "lexer.h"   /* for tokenlist */

/* The files a command reads from / writes to. NULL means no redirection. */
typedef struct {
	char *in_file;    /* from "< file" */
	char *out_file;   /* from "> file" */
} redirection;

/*
 * Part 6: I/O redirection.
 *
 * Finds "< file" and "> file" in tokens, stores the file names in *redir
 * and removes those tokens, so only the command and its arguments are
 * left for execv(). Either order works: "cmd < in > out" and
 * "cmd > out < in" give the same result.
 *
 * Also checks that the input file exists and is a regular file, so the
 * error is reported by the shell before any process is created.
 *
 * Returns 0 on success, or -1 after printing an error. Either way, the
 * caller must call free_redirection() afterwards.
 */
int parse_redirection(tokenlist *tokens, redirection *redir);

/*
 * Called in the child after fork() and before execv(). Points stdin at
 * the input file and stdout at the output file. The input is opened
 * first and read-only; the output is created or truncated with
 * permissions -rw-------.
 *
 * Returns 0 on success, or -1 after printing an error.
 */
int apply_redirection(const redirection *redir);

/* Frees the file names stored by parse_redirection(). */
void free_redirection(redirection *redir);

#endif
