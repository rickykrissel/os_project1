/*
 * execute.c
 *
 * Part 5: External command execution
 *
 * Built-ins are handled before this is called. search_path() (path.c)
 * turns the command name into a full path, then we fork() and execv().
 */

#define _POSIX_C_SOURCE 200809L

#include "execute.h"
#include "path.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int execute_external(tokenlist *tokens)
{
	if (tokens == NULL || tokens->size == 0)
		return -1;

	/* 1. Find the program in the parent, before forking, so a missing
	 *    command is reported without creating a process. */
	char *path = search_path(tokens->items[0]);
	if (path == NULL) {
		fprintf(stderr, "%s: command not found\n", tokens->items[0]);
		return -1;
	}

	/* search_path() returns paths with a '/' ("./a.out") unchecked,
	 * so make sure those exist and are executable here too. */
	if (access(path, X_OK) != 0) {
		perror(tokens->items[0]);
		free(path);
		return -1;
	}

	/* Anything still buffered (like the prompt) is written now, so it
	 * can't get mixed up with the child's output. */
	fflush(stdout);

	/* 2. Create the child process. */
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		free(path);
		return -1;
	}

	/* 3. Child: replace this process with the program. argv[0] stays
	 *    as the user typed it ("ls"), like bash does. */
	if (pid == 0) {
		execv(path, tokens->items);

		/* execv() only returns on failure. Exit right away, or the
		 * child would keep running as a second copy of the shell. */
		perror(tokens->items[0]);
		_exit(EXIT_FAILURE);
	}

	/* 4. Parent: wait for the child to finish before the next prompt. */
	int status;
	if (waitpid(pid, &status, 0) < 0)
		perror("waitpid");

	free(path);
	return 0;
}