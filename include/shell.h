#ifndef SHELL_H
#define SHELL_H

#include "jobs.h"

#define HISTORY_SIZE 3

typedef struct {
	job_table jobs;
	char *history[HISTORY_SIZE];
	size_t history_count;
} shell_state;

typedef enum {
	COMMAND_ERROR = -1,
	COMMAND_OK = 0,
	COMMAND_EXIT = 1
} command_result;

#endif
