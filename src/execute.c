#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "jobs.h"
#include "execute.h"

void handle_in(char *in) {
    int fdi;
    if (in) {
        if ((fdi = open(in, O_RDONLY)) == -1) {
            perror("Error opening the file");
            exit(EXIT_FAILURE);
        }
        dup2(fdi, STDIN_FILENO);
        close(fdi);
    }
}
void handle_out(char *out)
{
    int fdo;
    if (out) {
        if ((fdo = open(out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
            perror("Error opening the file");
            exit(EXIT_FAILURE);
        }
        dup2(fdo, STDOUT_FILENO);
        close(fdo);
    }

}

void exec_cmd(struct cmdline *l) {
    char **args = l->seq[0];
    char *cmd = args[0];
    int status;
    pid_t pid = fork();

    if (pid < 0) { perror("Error occurred in fork"); return; }
    if (pid == 0) {
        if (!strcmp(cmd, "jobs"))
        {
            print_jobs();
            exit(EXIT_SUCCESS);
        }
        handle_in(l->in);
        handle_out(l->out);
        if (execvp(cmd, args) == -1) {
            perror("Error running the command");
            exit(EXIT_FAILURE);
        }
    }
    else {
        if (l->bg)
            add_job(pid, l->seq);
        else {
            waitpid(pid, &status, 0);

        }
    }
}

void exec_pipe(struct cmdline *l) {
    char **cmd1 = l->seq[0];
    char **cmd2 = l->seq[1];
    int status;//, fdin, fdout;
    pid_t pid;
    int fd[2];

    if (pipe(fd) == -1) { perror("Can't open a pipe"); return; }

    pid = fork();
    if (pid == -1) { perror("Error in fork call"); return; }

    if (pid == 0)
    {
        // First child process - Will execute cmd1
        close(fd[0]); // Close unused read end
        dup2(fd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(fd[1]); // Close write end of the pipe
        handle_in(l->in);
        if (execvp(cmd1[0], cmd1) == -1) {
            perror("Error executing cmd1");
            exit(EXIT_FAILURE);
        }
    }
    else {
        // Parent process
        pid_t second_pid = fork();
        if (second_pid == -1) {
            perror("Error in second fork call");
            return;
        }

        if (second_pid == 0) {
            // Second child process - Will execute cmd2
            close(fd[1]); // Close unused write end
            dup2(fd[0], STDIN_FILENO); // Redirect stdin to pipe
            close(fd[0]); // Close read end of the pipe
            handle_out(l->out);
            if (execvp(cmd2[0], cmd2) == -1) {
                perror("Error executing cmd2");
                exit(EXIT_FAILURE);
            }
        } else {
            // Back in the parent process
            close(fd[0]);
            close(fd[1]);
            if (l->bg)
                add_job(pid, l->seq);
            else {
                waitpid(pid, &status, 0);
                waitpid(second_pid, &status, 0);
            }
        }
    }
}

void exec_mult_pipe(struct cmdline *l) {
    int i, num_cmds = 0;

    handle_in(l->in);
    while (l->seq[num_cmds] != NULL) {
        num_cmds++;
    }

    int pipes[num_cmds - 1][2]; // Array to hold the pipe file descriptors

    for (i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Error in opening the pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < num_cmds; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Error in fork call");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) { // Child process
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO); // Connect input to the previous pipe
            }
            if (i < num_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO); // Connect output to the next pipe
            }
            else
                handle_out(l->out);

            // Close all pipe file descriptors
            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(l->seq[i][0], l->seq[i]);
            perror("Error executing the command");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process: close all pipe file descriptors
    for (i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    for (i = 0; i < num_cmds; i++) {
        wait(NULL);
    }
}

void execute(struct cmdline *l) {
    if (!l->seq[0])
        return;
    if (!l->seq[1])
        exec_cmd(l);
    else if (!l->seq[2]) {
        exec_pipe(l);
    }
    else
        exec_mult_pipe(l);
}
