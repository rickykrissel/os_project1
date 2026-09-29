#include "jobs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int jobs_have_room(const job_table *jobs)
{
	for (size_t i = 0; i < MAX_JOBS; i++) {
		if (jobs->entries[i].count == 0)
			return 1;
	}
	return 0;
}

void jobs_add(job_table *jobs, const pid_t *pids, size_t count, char *command)
{
	for (size_t i = 0; i < MAX_JOBS; i++) {
		background_job *job = &jobs->entries[i];
		if (job->count != 0)
			continue;
		job->number = jobs->next_number++;
		job->count = count;
		job->command = command;
		job->last_pid = pids[count - 1];
		for (size_t j = 0; j < count; j++)
			job->pids[j] = pids[j];
		printf("[%lu] %ld\n", job->number, (long)job->last_pid);
		return;
	}
}

/* Reap every stage; the last stage can finish before the rest of a pipeline. */
static void collect_jobs(job_table *jobs, int options)
{
	for (size_t i = 0; i < MAX_JOBS; i++) {
		background_job *job = &jobs->entries[i];
		if (job->count == 0)
			continue;
		int running = 0;
		for (size_t j = 0; j < job->count; j++) {
			if (job->pids[j] == 0)
				continue;
			pid_t result;
			do {
				result = waitpid(job->pids[j], NULL, options);
			} while (result < 0 && errno == EINTR);
			if (result > 0 || (result < 0 && errno == ECHILD))
				job->pids[j] = 0;
			else {
				if (result < 0)
					perror("waitpid");
				running = 1;
			}
		}
		if (!running) {
			printf("[%lu] + done %s\n", job->number, job->command);
			free(job->command);
			job->command = NULL;
			job->count = 0;
		}
	}
}

void jobs_poll(job_table *jobs)
{
	collect_jobs(jobs, WNOHANG);
}

void jobs_wait(job_table *jobs)
{
	collect_jobs(jobs, 0);
}

void jobs_print(const job_table *jobs)
{
	int found = 0;
	for (size_t i = 0; i < MAX_JOBS; i++) {
		const background_job *job = &jobs->entries[i];
		if (job->count != 0) {
			printf("[%lu]+ %ld %s\n", job->number, (long)job->last_pid,
			       job->command);
			found = 1;
		}
	}
	if (!found)
		printf("No active background processes.\n");
}
