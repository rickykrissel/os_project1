#define _POSIX_C_SOURCE 200809L

#include "builtins.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int is_builtin(const char *command)
{
	return strcmp(command, "cd") == 0 || strcmp(command, "jobs") == 0 || strcmp(command, "exit") == 0;
}

static command_result change_directory(const tokenlist *tokens)
{
	if (tokens->size > 2) {
		fprintf(stderr, "cd: expected at most one argument\n");
		return COMMAND_ERROR;
	}

	const char *target;
	if (tokens->size == 2) {
		target = tokens->items[1];
	} 
	else {
		target = getenv("HOME");
	}

	if (target == NULL) {
		fprintf(stderr, "cd: HOME is not set\n");
		return COMMAND_ERROR;
	}
	//  reports targets that dont exist and arent directories
	if (chdir(target) != 0) {
		fprintf(stderr, "cd: %s: %s\n", target, strerror(errno));
		return COMMAND_ERROR;
	}

	
	size_t size = 256;
	char *cwd;

	for (;;) {
		cwd = malloc(size);
		if (cwd == NULL) {
			perror("cd: malloc");
			return COMMAND_ERROR;
		}
		if (getcwd(cwd, size) != NULL)
			break;

		int error = errno;

		free(cwd);
		if (error != ERANGE) {
			fprintf(stderr, "cd: getcwd: %s\n", strerror(error));
			return COMMAND_ERROR;
		}
		size *= 2;
	}

	int result = setenv("PWD", cwd, 1);
	free(cwd);
	if (result != 0) {
		perror("cd: setenv");
		return COMMAND_ERROR;
	}
	return COMMAND_OK;
}

command_result execute_builtin(tokenlist *tokens, shell_state *shell, int in_child)
{
	if (strcmp(tokens->items[0], "cd") == 0)
		return change_directory(tokens);
	if (tokens->size != 1) {
		fprintf(stderr, "%s: expected no arguments\n", tokens->items[0]);
		return COMMAND_ERROR;
	}
	if (strcmp(tokens->items[0], "jobs") == 0) {
		jobs_print(&shell->jobs);
		return COMMAND_OK;
	}
	if (!in_child)
		shell_finish(shell);
	return COMMAND_EXIT;
}

void history_record(shell_state *shell, const char *command)
{
	char *copy = strdup(command);
	if (copy == NULL) {
		perror("history: strdup");
		return;
	}
	if (shell->history_count == HISTORY_SIZE) {
		free(shell->history[0]);
		for (size_t i = 1; i < HISTORY_SIZE; i++)
			shell->history[i - 1] = shell->history[i];
			
		shell->history_count--;
	}
	shell->history[shell->history_count++] = copy;
}

void shell_finish(shell_state *shell)
{
	jobs_wait(&shell->jobs);
	if (shell->history_count == 0) {
		printf("No valid commands.\n");
		return;
	}
	printf("Last valid commands:\n");
	
	size_t first = shell->history_count < HISTORY_SIZE ? shell->history_count - 1 : 0;
	for (size_t i = first; i < shell->history_count; i++)
		printf("%s\n", shell->history[i]);
}

void history_free(shell_state *shell)
{
	for (size_t i = 0; i < shell->history_count; i++)
		free(shell->history[i]);
	shell->history_count = 0;
}
