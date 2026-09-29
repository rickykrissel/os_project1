#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>


typedef struct {
	char **items;   //token strings, followed by a NULL entry
	size_t size;    //number of tokens (excluding the NULL)
} tokenlist;


char *get_input(void);


//Tokenizes the line
tokenlist *get_tokens(const char *input);

//Frees a tokenlist and every token in it
void free_tokens(tokenlist *tokens);

#endif
