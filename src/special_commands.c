#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"

void handle_pipe(char **left_cmd, char **right_cmd)
{
    int fd[2]; /*fd[0] for read and fd[1] for write*/
    pid_t p;
    if (pipe(fd) == -1)
    {
        perror("pipefd: ");
        return;
    }

    p = fork();
    if (p < 0)
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    else if (p == 0)
    {
        dup2(fd[1], 1);

        close(fd[0]);
        close(fd[1]);

        exec_standard(left_cmd);
        // execvp(left_cmd[0], left_cmd);
        // perror(left_cmd[0]);
        // exit(EXIT_FAILURE);
    }

    p = fork();
    if (p < 0)
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    else if (p == 0)
    {
        dup2(fd[0], 0);

        close(fd[0]);
        close(fd[1]);
        exec_standard(right_cmd);
        // execvp(right_cmd[0], right_cmd);
        // perror(right_cmd[0]);
        // exit(EXIT_FAILURE);
    }

    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
    return;
}

void handle_output_redirect(char **left_cmd, char **right_cmd, char *redirect)
{
    if (fork() == 0)
    {
        int fd1, fd2;
        int flags = O_WRONLY | O_CREAT;
        if (strcmp(redirect, ">") == 0)
        {
            flags |= O_TRUNC;
            fd2 = STDOUT_FILENO;
        }
        else if (strcmp(redirect, ">>") == 0)
        {
            flags |= O_APPEND;
            fd2 = STDOUT_FILENO;
        }
        else if (strcmp(redirect, "2>") == 0)
        {
            flags |= O_TRUNC;
            fd2 = STDERR_FILENO;
        }
        else if (strcmp(redirect, "2>>") == 0)
        {
            flags |= O_APPEND;
            fd2 = STDERR_FILENO;
        }
        else
        {
            perror("Invalid output redirect operator");
            return;
        }
        fd1 = open(right_cmd[0], flags, 0644);
        if (fd1 < 0)
        {
            perror("File open error");
            return;
        }
        dup2(fd1, fd2);
        close(fd1);
        // append the rest from right_cmd to left_cmd (repair the array)
        int i, j;
        for (i = 0; left_cmd[i] != NULL; i++)
            ;
        for (j = 1; right_cmd[j] != NULL; j++)
        {
            left_cmd[i + j - 1] = right_cmd[j];
        }
        left_cmd[i + j - 1] = (char *)NULL;
        right_cmd[1] = (char *)NULL;
        exec_standard(left_cmd);
    }
    wait(NULL);
    return;
}

void handle_input_redirect(char **left_cmd, char **right_cmd)
{
    if (fork() == 0)
    {
        int fd;
        fd = open(right_cmd[0], O_RDONLY);
        if (fd < 0)
        {
            perror("File open error");
            return;
        }
        dup2(fd, 0);
        close(fd);
        int i, j;
        for (i = 0; left_cmd[i] != NULL; i++)
            ;
        for (j = 1; right_cmd[j] != NULL; j++)
        {
            left_cmd[i + j - 1] = right_cmd[j];
        }
        left_cmd[i + j - 1] = (char *)NULL;
        right_cmd[1] = (char *)NULL;
        exec_standard(left_cmd);
    }
    wait(NULL);
    return;
}

void handle_background(char **left_cmd, char **right_cmd)

// todo:
// if we do the sigaction one it does handle it well but it is same as wait
// as the sighandler is same as wait(NULL) so it is like we do not do it in parallel
// so why not create a separate thread to handle the background process and let the main thread continue to run without waiting for it at all, and the signal handler will just reap the background processes when they finish without affecting the main thread's execution at all
{
    if (fork() == 0)
    {
        exec_standard(left_cmd);
    }
    if (*right_cmd != NULL)
    {
        standard_command_run(right_cmd);
    }
    return;
}

void handle_semicolon(char **left_cmd, char **right_cmd)
{
    if (fork() == 0)
    {
        exec_standard(left_cmd);
    }
    wait(NULL);
    if (*right_cmd != NULL)
    {
        exec_standard(right_cmd);
    }
    return;
}

// This is for a simple special command, like a single pipe or a single redirect
void special_command_run(char **tokens, int which_special)
{

    int len = MAX_TOKENS / 2 + 1;
    int i;
    char **left_cmd = malloc(sizeof(char *) * (len + 1));
    char **right_cmd = malloc(sizeof(char *) * (len + 1));

    for (i = 0; i < which_special; i++)
    {
        left_cmd[i] = tokens[i];
    }
    left_cmd[i] = (char *)NULL;

    for (i = which_special + 1; tokens[i] != NULL; i++)
    {
        right_cmd[i - which_special - 1] = tokens[i];
    }
    right_cmd[i - which_special - 1] = (char *)NULL;

    // special command function choosing
    if (strncmp(tokens[which_special], "|", 1) == 0)
    {
        handle_pipe(left_cmd, right_cmd);
    }
    else if (strncmp(tokens[which_special], ">", 1) == 0 || strncmp(tokens[which_special], ">>", 2) == 0 || strncmp(tokens[which_special], "2>", 2) == 0 || strncmp(tokens[which_special], "2>>", 3) == 0)
    {
        handle_output_redirect(left_cmd, right_cmd, tokens[which_special]);
    }
    else if (strncmp(tokens[which_special], "<", 1) == 0)
    {
        handle_input_redirect(left_cmd, right_cmd);
    }
    else if (strncmp(tokens[which_special], "&", 1) == 0)
    {
        handle_background(left_cmd, right_cmd);
    }
    else if (strncmp(tokens[which_special], "&&", 2) == 0)
    {
        handle_and(left_cmd, right_cmd);
    }
    else if (strncmp(tokens[which_special], "||", 2) == 0)
    {
        handle_or(left_cmd, right_cmd);
    }
    else if (strncmp(tokens[which_special], ";", 1) == 0)
    {
        handle_semicolon(left_cmd, right_cmd);
    }
    else
    {
        perror("Invalid special command");
        return;
    }

    free(right_cmd);
    free(left_cmd);
    return;
}

// this is for handling multiple special commands in one go
void special_commands_run(char **tokens)
{
    int *which_special = malloc(MAX_TOKENS * sizeof(int));
    int count = 0, i, j;
    for (i = 0; tokens[i] != NULL; i++)
    {
        for (j = 0; special_commands[j] != NULL; j++)
        {
            if (strcmp(tokens[i], special_commands[j]) == 0)
            {
                which_special[count] = i;
                count++;
                break;
            }
        }
    }
    // for (i=0; i < count; i++){
    //     printf("Special command %d at index %d\n", i, which_special[i]);
    // }
    if (count == 1)
    {
        special_command_run(tokens, which_special[0]);
    }
    else
    {
        // to be continued
    }

    free(which_special);
    return;
}
// ---------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// Signal handler function to reap zombie processes
void sigchld_handler(int signum)
{
    pid_t pid;
    int status;

    // Reap all zombie processes
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("Parent process reaped child process with PID %d.\n", pid);
    }
}

int main()
{
    pid_t child_pid;
    struct sigaction sa;

    // Register the signal handler for SIGCHLD
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Create a child process
    child_pid = fork();

    if (child_pid < 0)
    {
        // Fork failed
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (child_pid == 0)
    {
        // Child process
        printf("Child process is running.\n");
        sleep(2);           // Simulate some work
        exit(EXIT_SUCCESS); // Exit the child process
    }
    else
    {
        // Parent process
        printf("Parent process is running and will continue to run.\n");
        // Parent process continues running and will periodically reap zombie processes 
        while (1)
        {
            sleep(5); // Sleep for a while to allow child processes to terminate
            printf("Parent process is still running.\n");
        }
    }

    return 0;
}