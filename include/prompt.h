#ifndef PROMPT_H
#define PROMPT_H

/*
 * Prints the shell prompt in the form USER@MACHINE:PWD> with no
 * trailing newline, so the user types on the same line.
 */
void print_prompt(void);

#endif