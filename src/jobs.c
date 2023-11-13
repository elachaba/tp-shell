#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jobs.h"

struct bg_process * JOBS = NULL;

char *listToString(char ***seq) {
    int cmdLen = 0;

    if (!seq || !seq[0]) return NULL;

    for (int i = 0; seq[i]; ++i) {
        for (int j = 0; seq[i][j]; j++)
            cmdLen += strlen(seq[i][j]) + 1; // +1 for space or null terminator
    }

    char *cmd = malloc(cmdLen);
    if (!cmd) {
        perror("Error malloc for cmd assembly\n");
        return NULL;
    }

    cmd[0] = '\0';
    for (int i = 0; seq[i]; i++) {
        if (i > 0)
            strcat(cmd, " | ");
        for (int j = 0; seq[i][j]; j++) {
            strcat(cmd, seq[i][j]);
            if (seq[i][j + 1]) {
                strcat(cmd, " "); // Add a space if not the last argument
            }
        }

    }

    return cmd;
}

void add_job(pid_t pid, char*** seq)
{
    struct bg_process *process = malloc(sizeof(struct bg_process));
    char *cmd;

    if (!process) { perror("Error malloc of a new background process\n"); exit(EXIT_FAILURE); }
    cmd = listToString(seq);
    process->pid = pid;
    process->cmd = cmd;
    process->next = JOBS;
    process->flag = RUNNING;
    JOBS = process;
}


void print_jobs()
{
    struct bg_process *current = JOBS;
    int processNumber = 1; // To number the jobs in the list

    if (current == NULL)
        return;

    while (current != NULL) {
        printf("[%d]\t%d\t\t%s &\n", processNumber, current->pid, current->cmd);
        current = current->next;
        processNumber++;
    }
}

void remove_job(pid_t pid) {
    struct bg_process *current = JOBS;

    while (current != NULL) {
        if (current->pid == pid) {
            current->flag = 1; // Mark as completed
            return;
        }
        current = current->next;
    }
}

// Function to remove a job from the job list
void print_and_remove_completed_jobs() {
    struct bg_process *current = JOBS;
    struct bg_process *prev = NULL;
    while (current != NULL) {
        if (current->flag) {
            printf("Done\t%d\t%s\n", current->pid, current->cmd);
            remove_job(current->pid);  // Remove the completed job

            // Need to handle prev pointer correctly here
            if (prev) {
                prev->next = current->next;
            } else {
                JOBS = current->next;
            }
            free(current->cmd);
            free(current);
            current = (prev) ? prev->next : JOBS;
            continue;
        }
        prev = current;
        current = current->next;
    }
}


void mark_job_completed(pid_t pid) {
    struct bg_process *current = JOBS;
    while (current != NULL) {
        if (current->pid == pid) {
            current->flag = DONE;  // Mark as completed
            return;
        }
        current = current->next;
    }
}



void free_job_list() {
    struct bg_process *current = JOBS;
    while (current != NULL) {
        struct bg_process *next = current->next;
        free(current->cmd); // Free the command string
        free(current);      // Free the job structure
        current = next;
    }
    JOBS = NULL; // Set JOBS to NULL after freeing
}