#define _POSIX_C_SOURCE 200809L   /* for gethostname() */

#include "prompt.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* $USER, or "unknown" if it is not set */
static const char *prompt_user(void)
{
	const char *user = getenv("USER");
	return user != NULL ? user : "unknown";
}

/*
 * $MACHINE if it is set. Most systems do not export it, so fall back
 * to the host name reported by the kernel.
 */
static const char *prompt_machine(void)
{
	static char host[HOST_NAME_MAX + 1];
	const char *machine = getenv("MACHINE");

	if (machine != NULL)
		return machine;
	if (gethostname(host, sizeof host) != 0)
		return "unknown";
	host[HOST_NAME_MAX] = '\0';
	return host;
}

/*
 * The absolute working directory. getcwd() is used rather than $PWD
 * because $PWD is not updated when the shell itself changes directory
 * with cd; $PWD is only a fallback if getcwd() fails.
 */
static const char *prompt_pwd(void)
{
	static char cwd[PATH_MAX];
	const char *pwd;

	if (getcwd(cwd, sizeof cwd) != NULL)
		return cwd;
	pwd = getenv("PWD");
	return pwd != NULL ? pwd : "?";
}

void print_prompt(void)
{
	printf("%s@%s:%s>", prompt_user(), prompt_machine(), prompt_pwd());
	fflush(stdout);   /* no newline, so stdout would not flush on its own */
}