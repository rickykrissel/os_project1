/*
 * path.c
 *
 * Part 4: $PATH search  (ls -> /usr/bin/ls)
 *
 * Built-ins are handled before this is called, so only external
 * commands are looked up here.
 */

#define _POSIX_C_SOURCE 200809L   /* for strdup() */

#include "path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Returns 1 if path is a regular file we are allowed to execute. */
static int is_executable(const char *path)
{
	struct stat st;

	if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
		return 0;
	return access(path, X_OK) == 0;
}

/* Returns a malloc'd copy of s, exiting if memory runs out. */
static char *copy_string(const char *s)
{
	char *copy = strdup(s);
	if (copy == NULL) {
		perror("strdup");
		exit(EXIT_FAILURE);
	}
	return copy;
}

char *search_path(const char *cmd)
{
	if (cmd == NULL || cmd[0] == '\0')
		return NULL;

	/* "./a.out", "/bin/ls", "bin/shell": no search, execv() gets it as-is */
	if (strchr(cmd, '/') != NULL)
		return copy_string(cmd);

	const char *path = getenv("PATH");
	if (path == NULL)
		return NULL;

	/* strtok() would modify $PATH itself, so split a copy of it */
	char *dirs = copy_string(path);
	char *result = NULL;

	for (char *dir = strtok(dirs, ":"); dir != NULL; dir = strtok(NULL, ":")) {
		/* room for dir + "/" + cmd + '\0' */
		char *candidate = malloc(strlen(dir) + strlen(cmd) + 2);
		if (candidate == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		sprintf(candidate, "%s/%s", dir, cmd);

		if (is_executable(candidate)) {
			result = candidate;
			break;
		}
		free(candidate);
	}

	free(dirs);
	return result;
}
