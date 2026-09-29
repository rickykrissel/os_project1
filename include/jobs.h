#ifndef JOBS_H
#define JOBS_H

#include <stddef.h>
#include <sys/types.h>

#define MAX_JOBS 10
#define MAX_PIPELINE_COMMANDS 3

typedef struct {
	unsigned long number;
	pid_t pids[MAX_PIPELINE_COMMANDS];
	pid_t last_pid;
	size_t count;
	char *command;
} background_job;

typedef struct {
	background_job entries[MAX_JOBS];
	unsigned long next_number;
} job_table;

int jobs_have_room(const job_table *jobs);
//Takes ownership of command. The caller checks capacity before forking.
void jobs_add(job_table *jobs, const pid_t *pids, size_t count, char *command);
void jobs_poll(job_table *jobs);
void jobs_print(const job_table *jobs);
void jobs_wait(job_table *jobs);

#endif
