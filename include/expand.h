#ifndef EXPAND_H
#define EXPAND_H

#include "lexer.h"

//Replaces $VAR and ~ tokens in place with their expanded values
void expand_tokens(tokenlist *tokens);

#endif
