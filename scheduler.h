#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

void scheduler_load_default_jobs(Job jobs[], int *n);
int scheduler_load_file(Job jobs[], int *n, const char *path);
int scheduler_generate_random(Job jobs[], int *n, int count, unsigned seed);
int scheduler_input_jobs(Job jobs[], int *n);
int scheduler_add_job(Job jobs[], int *n);
int scheduler_edit_job_priority(Job jobs[], int n);
void scheduler_display_jobs(const Job jobs[], int n);

int scheduler_set_quantum(int q);
int scheduler_get_quantum(void);
int scheduler_input_quantum(void);

/* Each returns 0 on success, 1 if there are no jobs to schedule. */
int scheduler_run_fcfs(Job jobs[], int n);
int scheduler_run_rr(Job jobs[], int n);
int scheduler_run_priority(Job jobs[], int n);
int scheduler_compare_all(const Job jobs[], int n);

#endif
