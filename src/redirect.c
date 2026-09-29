
#define _POSIX_C_SOURCE 200809L  

#include "redirect.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


#define OUT_FILE_MODE (S_IRUSR | S_IWUSR)

// returns 1 if token is one of the shell operators
static int is_operator_token(const char *token)
{
	return strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, "|") == 0 || strcmp(token, "&") == 0;
}

static int check_input_file(const char *file)
{
	struct stat st;

	if (stat(file, &st) != 0) {
		perror(file);
		return -1;
	}
	if (!S_ISREG(st.st_mode)) {
		fprintf(stderr, "%s: not a regular file\n", file);
		return -1;
	}
	return 0;
}

int parse_redirection(tokenlist *tokens, redirection *redir)
{
	size_t kept = 0; 
	
	int status = 0;

	redir->in_file = NULL;
	redir->out_file = NULL;

	for (size_t i = 0; i < tokens->size; i++) {
		char *token = tokens->items[i];
		int is_in = strcmp(token, "<") == 0;
		int is_out = strcmp(token, ">") == 0;

		if (!is_in && !is_out) {
			tokens->items[kept++] = token;
			continue;
		}

		if (i + 1 >= tokens->size ||
		    is_operator_token(tokens->items[i + 1])) {
			if (status == 0)  
				fprintf(stderr, "error: missing file after '%s'\n",
					token);
			status = -1;
			free(token);
			continue;
		}

		char **target = is_in ? &redir->in_file : &redir->out_file;
		free(*target);

		*target = tokens->items[++i];
		free(token);
	}

	tokens->size = kept;
	tokens->items[kept] = NULL;  // keep the list NULL-terminated for execv()

	if (status == 0 && kept == 0 &&
	    (redir->in_file != NULL || redir->out_file != NULL)) {
		fprintf(stderr, "syntax error: missing command\n");
		status = -1;
	}

	// the input file is dealt with first, before the command runs.
	if (status == 0 && redir->in_file != NULL)
		status = check_input_file(redir->in_file);

	return status;
}

int apply_redirection(const redirection *redir)
{
	// readonly so file cant be modified
	if (redir->in_file != NULL) {
		int fd = open(redir->in_file, O_RDONLY);
		if (fd < 0) {
			perror(redir->in_file);
			return -1;
		}
		if (dup2(fd, STDIN_FILENO) < 0) {
			perror("redirect: dup2");
			close(fd);
			return -1;
		}
		close(fd);
	}

		// create file if its missing, overwrite  if present
	if (redir->out_file != NULL) {
		int fd = open(redir->out_file, O_WRONLY | O_CREAT | O_TRUNC,
			      OUT_FILE_MODE);
		if (fd < 0) {
			perror(redir->out_file);
			return -1;
		}
		if (fchmod(fd, OUT_FILE_MODE) < 0 || dup2(fd, STDOUT_FILENO) < 0) {
			perror("redirect");
			close(fd);
			
			return -1;
		}
		close(fd);
	}

	return 0;
}

void free_redirection(redirection *redir)
{
	free(redir->in_file);
	free(redir->out_file);
	redir->in_file = NULL;
	redir->out_file = NULL;
}
