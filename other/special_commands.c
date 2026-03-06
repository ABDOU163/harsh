#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"

// This is for a simple special command, like a single pipe or a single redirect
void special_command_run(char **tokens, int which_special){

    int len = MAX_TOKENS_LIMIT / 2 + 1;
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
    if (strcmp(tokens[which_special], "||") == 0)
    {
        handle_or(left_cmd, right_cmd);
    }
    else if (strcmp(tokens[which_special], ">") == 0 || strcmp(tokens[which_special], "2>") == 0 || strcmp(tokens[which_special], ">>") == 0 || strcmp(tokens[which_special], "2>>") == 0)
    {
        handle_output_redirect(tokens, which_special);
    }
    else if (strcmp(tokens[which_special], "<") == 0)
    {
        handle_input_redirect(tokens, which_special);
    }
    else if (strcmp(tokens[which_special], "&&") == 0)
    {
        handle_and(left_cmd, right_cmd);
    }
    else if (strcmp(tokens[which_special], "&") == 0)
    {
        handle_background(left_cmd, right_cmd);
    }
    else if (strcmp(tokens[which_special], "|") == 0)
    {
        handle_pipe(left_cmd, right_cmd);
    }
    else if (strcmp(tokens[which_special], ";") == 0)
    {
        handle_semicolon(left_cmd, right_cmd);
    }
    else
    {
        perror("Invalid special command");
    }

    free(right_cmd);
    free(left_cmd);
    return;
}


void handle_pipe(char **left_cmd, char **right_cmd){
    int fd[2]; /*fd[0] for read and fd[1] for write*/
    pid_t p;
    if (pipe(fd) == -1){
        perror("pipefd: ");
        return;
    }

    if (fork() == 0){
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        exec_standard(left_cmd);
    }
    if (fork() == 0){
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        exec_standard(right_cmd);
    }

    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
    return;
}

void handle_output_redirect(char **tokens, int which_special){
    if (fork() == 0){
        int fd1, fd2;
        int flags = O_WRONLY | O_CREAT;
        char *redirect = tokens[which_special];
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
            exit(EXIT_FAILURE);
        }
        fd1 = open(tokens[which_special + 1], flags, 0644);
        if (fd1 < 0)
        {
            perror("File open error");
            exit(EXIT_FAILURE);
        }
        dup2(fd1, fd2);
        close(fd1);
        // get the command to execute
        // malloc an array of all tokens excluding the redirect operator and the file name
        int i, j;
        j=0;
        char **cmd_tokens = malloc(sizeof(char *) * (MAX_TOKENS_LIMIT + 1));
        for (i = 0; tokens[i] != NULL; i++){
            if (i == which_special || i == which_special + 1){
                continue;
            }
            cmd_tokens[j++] = tokens[i];
        }
        cmd_tokens[j] = (char *)NULL;
        exec_standard(cmd_tokens);
    }
    wait(NULL);
    return;
}

void handle_input_redirect(char **tokens, int which_special){
    if (fork() == 0)
    {
        int fd;
        fd = open(tokens[which_special + 1], O_RDONLY);
        if (fd < 0)
        {
            perror("File open error");
            exit(EXIT_FAILURE);
        }
        dup2(fd, 0);
        close(fd);
        // get the command to execute
        // malloc an array of all tokens excluding the redirect operator and the file name
        int i, j;
        j=0;
        char **cmd_tokens = malloc(sizeof(char *) * (MAX_TOKENS_LIMIT + 1));
        for (i = 0; tokens[i] != NULL; i++){
            if (i == which_special || i == which_special + 1){
                continue;
            }
            cmd_tokens[j++] = tokens[i];
        }
        cmd_tokens[j] = (char *)NULL;
        exec_standard(cmd_tokens);
    }
    wait(NULL);
    return;
}

void handle_background(char **left_cmd, char **right_cmd){
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

void handle_semicolon(char **left_cmd, char **right_cmd){
    if (*left_cmd != NULL){
        standard_command_run(left_cmd);
    }
    if (*right_cmd != NULL){
        standard_command_run(right_cmd);
    }
    return;
}

void handle_or(char **left_cmd, char **right_cmd){
    int status = standard_command_run(left_cmd);
    if (status != 0){
        standard_command_run(right_cmd);
    }
    return;
}

void handle_and(char **left_cmd, char **right_cmd){
    int status = standard_command_run(left_cmd);
    if (status == 0){
        standard_command_run(right_cmd);
    }
    return;
}