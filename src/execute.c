/* Parts 5-8: command preparation, concurrent pipelines, and background jobs. */
#define _POSIX_C_SOURCE 200809L

#include "execute.h"
#include "builtins.h"
#include "path.h"
#include "redirect.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
	tokenlist *tokens;
	redirection redir;
	char *path;
	int builtin;
} command_stage;

static void free_stages(command_stage *stages, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		free_tokens(stages[i].tokens);
		free_redirection(&stages[i].redir);
		free(stages[i].path);
	}
}

/* Give each stage its own token list so redirection can remove its operands. */
static tokenlist *copy_stage(const tokenlist *tokens, size_t first, size_t end)
{
	tokenlist *copy = calloc(1, sizeof *copy);
	if (copy == NULL)
		return NULL;
	copy->items = calloc(end - first + 1, sizeof *copy->items);
	if (copy->items == NULL) {
		free(copy);
		return NULL;
	}
	for (size_t i = first; i < end; i++) {
		copy->items[copy->size] = strdup(tokens->items[i]);
		if (copy->items[copy->size] == NULL) {
			free_tokens(copy);
			return NULL;
		}
		copy->size++;
	}
	return copy;
}

static int prepare_stage(command_stage *stage)
{
	if (parse_redirection(stage->tokens, &stage->redir) != 0)
		return -1;
	const char *name = stage->tokens->items[0];
	stage->builtin = is_builtin(name);
	if (stage->builtin)
		return 0;
	stage->path = search_path(name);
	if (stage->path == NULL) {
		fprintf(stderr, "%s: command not found\n", name);
		return -1;
	}
	struct stat st;
	if (stat(stage->path, &st) != 0) {
		perror(name);
		return -1;
	}
	if (!S_ISREG(st.st_mode)) {
		fprintf(stderr, "%s: not a regular executable file\n", name);
		return -1;
	}
	if (access(stage->path, X_OK) != 0) {
		perror(name);
		return -1;
	}
	return 0;
}

static int parse_pipeline(const tokenlist *tokens, command_stage *stages,
			  size_t *count, int *background)
{
	size_t end = tokens->size;
	*background = strcmp(tokens->items[end - 1], "&") == 0;
	if (*background)
		end--;
	size_t first = 0;
	for (size_t i = 0; i <= end; i++) {
		if (i < end && strcmp(tokens->items[i], "&") == 0) {
			fprintf(stderr, "syntax error: '&' must end the command\n");
			return -1;
		}
		if (i < end && strcmp(tokens->items[i], "|") != 0)
			continue;
		if (i == first) {
			fprintf(stderr, "syntax error: missing pipeline command\n");
			return -1;
		}
		if (*count == MAX_PIPELINE_COMMANDS) {
			fprintf(stderr, "syntax error: at most two pipes are supported\n");
			return -1;
		}
		command_stage *stage = &stages[(*count)++];
		stage->tokens = copy_stage(tokens, first, i);
		if (stage->tokens == NULL) {
			perror("pipeline: allocation");
			return -1;
		}
		if (prepare_stage(stage) != 0)
			return -1;
		first = i + 1;
	}
	return 0;
}

/* Parent built-ins must restore the shell's descriptors even on errors. */
static command_result run_parent_builtin(command_stage *stage, shell_state *shell)
{
	if (stage->redir.in_file == NULL && stage->redir.out_file == NULL)
		return execute_builtin(stage->tokens, shell, 0);
	fflush(NULL);
	int saved_in = dup(STDIN_FILENO);
	int saved_out = dup(STDOUT_FILENO);
	if (saved_in < 0 || saved_out < 0) {
		perror("dup");
		if (saved_in >= 0)
			close(saved_in);
		if (saved_out >= 0)
			close(saved_out);
		return COMMAND_ERROR;
	}
	command_result result = COMMAND_ERROR;
	if (apply_redirection(&stage->redir) == 0)
		result = execute_builtin(stage->tokens, shell, 0);
	fflush(NULL);
	if (dup2(saved_in, STDIN_FILENO) < 0)
		perror("restore stdin");
	if (dup2(saved_out, STDOUT_FILENO) < 0)
		perror("restore stdout");
	close(saved_in);
	close(saved_out);
	return result;
}

static void close_pipes(int pipes[][2], size_t count)
{
	for (size_t i = 0; i < count; i++) {
		close(pipes[i][0]);
		close(pipes[i][1]);
	}
}

static void wait_children(const pid_t *pids, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		pid_t result;
		do {
			result = waitpid(pids[i], NULL, 0);
		} while (result < 0 && errno == EINTR);
		if (result < 0)
			perror("waitpid");
	}
}

/* The close-on-exec pipe distinguishes launch failures from program exit codes. */
static void child_failed(int error_fd)
{
	char failed = 1;
	while (write(error_fd, &failed, 1) < 0 && errno == EINTR)
		;
	_exit(EXIT_FAILURE);
}

static void run_child(command_stage *stages, size_t index, size_t count,
		      int pipes[][2], int error_fd, shell_state *shell)
{
	if ((index > 0 && dup2(pipes[index - 1][0], STDIN_FILENO) < 0) ||
	    (index + 1 < count && dup2(pipes[index][1], STDOUT_FILENO) < 0)) {
		perror("pipeline: dup2");
		child_failed(error_fd);
	}
	close_pipes(pipes, count - 1);
	if (apply_redirection(&stages[index].redir) != 0)
		child_failed(error_fd);
	if (stages[index].builtin) {
		command_result result = execute_builtin(stages[index].tokens, shell, 1);
		fflush(NULL);
		if (result == COMMAND_ERROR)
			child_failed(error_fd);
		_exit(EXIT_SUCCESS);
	}
	execv(stages[index].path, stages[index].tokens->items);
	perror(stages[index].tokens->items[0]);
	child_failed(error_fd);
}

/* Keep the original spelling of job commands, omitting the final '&'. */
static char *job_command(const char *line)
{
	size_t length = strlen(line);
	while (length > 0 && isspace((unsigned char)line[length - 1]))
		length--;
	if (length > 0 && line[length - 1] == '&')
		length--;
	while (length > 0 && isspace((unsigned char)line[length - 1]))
		length--;
	return strndup(line, length);
}

static command_result launch_pipeline(command_stage *stages, size_t count,
				      int background, const char *line, shell_state *shell)
{
	char *command = NULL;
	if (background) {
		if (!jobs_have_room(&shell->jobs)) {
			fprintf(stderr, "background: at most ten jobs may run at once\n");
			return COMMAND_ERROR;
		}
		command = job_command(line);
		if (command == NULL) {
			perror("background: allocation");
			return COMMAND_ERROR;
		}
	}

	int pipes[MAX_PIPELINE_COMMANDS - 1][2];
	size_t pipe_count = 0;
	for (; pipe_count + 1 < count; pipe_count++) {
		if (pipe(pipes[pipe_count]) != 0) {
			perror("pipe");
			close_pipes(pipes, pipe_count);
			free(command);
			return COMMAND_ERROR;
		}
	}
	int errors[2];
	if (pipe(errors) != 0) {
		perror("pipe");
		close_pipes(pipes, pipe_count);
		free(command);
		return COMMAND_ERROR;
	}
	if (fcntl(errors[1], F_SETFD, FD_CLOEXEC) < 0) {
		perror("fcntl");
		close(errors[0]);
		close(errors[1]);
		close_pipes(pipes, pipe_count);
		free(command);
		return COMMAND_ERROR;
	}

	fflush(NULL);
	pid_t pids[MAX_PIPELINE_COMMANDS];
	size_t started = 0;
	for (; started < count; started++) {
		pid_t pid = fork();
		if (pid < 0) {
			perror("fork");
			break;
		}
		if (pid == 0) {
			close(errors[0]);
			run_child(stages, started, count, pipes, errors[1], shell);
		}
		pids[started] = pid;
	}
	close_pipes(pipes, pipe_count);
	close(errors[1]);
	int failed = started != count;
	if (!failed) {
		char error;
		ssize_t bytes;
		/* All stages are forked before reading or waiting, avoiding pipe deadlocks. */
		while ((bytes = read(errors[0], &error, 1)) != 0) {
			if (bytes < 0 && errno == EINTR)
				continue;
			failed = 1;
			if (bytes < 0) {
				perror("startup: read");
				break;
			}
		}
	}
	close(errors[0]);
	if (failed) {
		/* A partially launched pipeline must not leave untracked children behind. */
		for (size_t i = 0; i < started; i++)
			kill(pids[i], SIGTERM);
		wait_children(pids, started);
		free(command);
		return COMMAND_ERROR;
	}
	if (background)
		jobs_add(&shell->jobs, pids, count, command);
	else
		wait_children(pids, count);
	return COMMAND_OK;
}

command_result execute_command(tokenlist *tokens, const char *line, shell_state *shell)
{
	if (tokens == NULL || tokens->size == 0)
		return COMMAND_ERROR;
	command_stage stages[MAX_PIPELINE_COMMANDS];
	memset(stages, 0, sizeof stages);
	size_t count = 0;
	int background = 0;
	command_result result = COMMAND_ERROR;
	if (parse_pipeline(tokens, stages, &count, &background) == 0) {
		if (count == 1 && !background && stages[0].builtin)
			result = run_parent_builtin(&stages[0], shell);
		else
			result = launch_pipeline(stages, count, background, line, shell);
	}
	free_stages(stages, count);
	return result;
}
