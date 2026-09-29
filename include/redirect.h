#ifndef REDIRECT_H
#define REDIRECT_H

#include "lexer.h" //for tokenlist

//input/output files for a command. NULL menas no redirect
typedef struct {
	char *in_file;
	char *out_file; 
} redirection;

int parse_redirection(tokenlist *tokens, redirection *redir);

//In the child, before execv(): points stdin/stdout at the files 0 or -1
int apply_redirection(const redirection *redir);


void free_redirection(redirection *redir);

#endif
