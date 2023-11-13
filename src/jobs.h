#ifndef _JOBS
#define _JOBS

#include <stdlib.h>

#define RUNNING 0
#define DONE 1

struct bg_process {
    char *cmd;
    pid_t pid;
    struct bg_process *next;
    int flag;
};


void print_jobs();
void add_job(pid_t pid, char ***seq);
void mark_job_completed(pid_t pid);
void print_and_remove_completed_jobs();
void free_job_list();

#endif