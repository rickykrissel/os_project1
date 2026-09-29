
#define _POSIX_C_SOURCE 200809L   
#include "path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int is_executable(const char *path)
{
	struct stat st;

	if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
		return 0;
	return access(path, X_OK) == 0;
}

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

	if (strchr(cmd, '/') != NULL)
		return copy_string(cmd);

	const char *path = getenv("PATH");
	if (path == NULL)
		return NULL;

	char *dirs = copy_string(path);

	char *result = NULL;

	for (char *dir = strtok(dirs, ":"); dir != NULL; dir = strtok(NULL, ":")) {
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
