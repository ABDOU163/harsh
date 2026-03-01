#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"

int setup_redirection_fd(char **tokens, int which_special){
    int fd;
    int to_fd=-1;
    char *redirect = tokens[which_special];
    int flags = O_WRONLY | O_CREAT;
    if (strcmp(redirect, ">>") == 0 || strcmp(redirect, "2>>") == 0){
        flags |= O_APPEND;
    } else if (strcmp(redirect, ">") == 0 || strcmp(redirect, "2>") == 0){
        flags |= O_TRUNC;
    }
    if (strcmp(redirect, "<") == 0){
        to_fd = STDIN_FILENO;
        fd = open(tokens[which_special + 1], O_RDONLY);
    }
    else if (strcmp(redirect, ">") == 0 || strcmp(redirect, ">>") == 0){
        to_fd = STDOUT_FILENO;
        fd = open(tokens[which_special + 1], flags, 0644);
    }
    else if (strcmp(redirect, "2>") == 0 || strcmp(redirect, "2>>") == 0){
        to_fd = STDERR_FILENO;
        fd = open(tokens[which_special + 1], flags, 0644);
    }
    else{
        perror("Invalid redirect operator");
        return -1;
    }

    if (fd < 0){
        perror("File open error");
        return -1;
    }
    if (dup2(fd, to_fd) < 0){
        perror("dup2 failed");
        close(fd);
        return -1;
    }
    if (close(fd) < 0){
        perror("File close error");
        return -1;
    }
    return 0;
}

// always run this in a child process, except for built-in commands, which we will handle separately by saving and restoring fds
void setup_redirect_execute(char **tokens, int *which_special, char **cmd_tokens, int count){
    for (int i=0; i < count; i++){
        if (setup_redirection_fd(tokens, which_special[i]) < 0){
            fprintf(stderr, "Redirection setup failed: %s %s\n", tokens[which_special[i]], tokens[which_special[i] + 1]);
            exit(EXIT_FAILURE);
        }
    }
    if (*cmd_tokens != NULL){
        exec_standard(cmd_tokens);
    }
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
        char **cmd_tokens = malloc(sizeof(char *) * (MAX_TOKENS + 1));
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
        char **cmd_tokens = malloc(sizeof(char *) * (MAX_TOKENS + 1));
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


void save_fds(int *saved_fds){
    saved_fds[0] = dup(STDIN_FILENO);
    saved_fds[1] = dup(STDOUT_FILENO);
    saved_fds[2] = dup(STDERR_FILENO);
    if (saved_fds[0] < 0 || saved_fds[1] < 0 || saved_fds[2] < 0){
        perror("Failed to save file descriptors");
        exit(EXIT_FAILURE);
    }
}

void restore_fds(int *saved_fds){
    dup2(saved_fds[0], STDIN_FILENO);
    dup2(saved_fds[1], STDOUT_FILENO);
    dup2(saved_fds[2], STDERR_FILENO);
    close(saved_fds[0]);
    close(saved_fds[1]);
    close(saved_fds[2]);
}

void get_cmd_tokens(char **tokens, int *which_special, int *count, char **cmd_tokens){
    int i, j, k;
    for (i = 0; tokens[i] != NULL; i++){
        for (j = 0; redirects[j] != NULL; j++){
            if (strcmp(tokens[i], redirects[j]) == 0){
                which_special[(*count)++] = i;
                break;
            }
        }
    }
    which_special[*count] = -1; 
    k=0;
    j=0;
    i=0;
    for (i = 0; tokens[i] != NULL && k<*count; i++){
        if (i == which_special[k]){
            continue;  
        }
        if (i == which_special[k] + 1){
            k++;       
            continue;
        }
        cmd_tokens[j++] = tokens[i];
    }
    for (; tokens[i] != NULL; i++){
        cmd_tokens[j++] = tokens[i];
    }
    cmd_tokens[j] = (char *)NULL;
}

// no need to fork to use this function
// if you want to fork you can use setup_redirect_execute
void multiple_redirects_run(char **tokens){
    int *which_special = malloc(MAX_TOKENS * sizeof(int));
    int count = 0;
    char **cmd_tokens = malloc(sizeof(char *) * (MAX_TOKENS + 1));
    get_cmd_tokens(tokens, which_special, &count, cmd_tokens);

    if (strcmp(*cmd_tokens, "exit") == 0 || strcmp(*cmd_tokens, "cd") ==0){
        int fds[3];
        save_fds(fds);
        setup_redirect_execute(tokens, which_special, cmd_tokens, count);
        restore_fds(fds);
    } else{
        if (fork() == 0){
            setup_redirect_execute(tokens, which_special, cmd_tokens, count);
        }
        wait(NULL);
    }
    free(cmd_tokens);
    free(which_special);
    return;
}


bool pipe_left(char **curr_tokens, int *which){
    for (int i=0; curr_tokens[i] != NULL; i++){
        if (strcmp(curr_tokens[i], "|") == 0){
            *which = i;
            return true;
        }
    }
    return false;
}

void handle_multiple_pipes(char **tokens){
    // Count pipe operators and collect their positions
    int pipe_positions[MAX_TOKENS];
    int pipe_count = 0;
    for (int i = 0; tokens[i] != NULL; i++){
        if (strcmp(tokens[i], "|") == 0){
            pipe_positions[pipe_count++] = i;
        }
    }

    // If no pipes, just run with multiple redirects
    if (pipe_count == 0){
        multiple_redirects_run(tokens);
        return;
    }

    int num_segments = pipe_count + 1;

    // Nullify pipe tokens in-place to create natural NULL-terminated segments
    // and collect pointers to the start of each segment
    char **seg_starts[num_segments];
    seg_starts[0] = tokens;
    for (int p = 0; p < pipe_count; p++){
        tokens[pipe_positions[p]] = NULL;  // nullify the "|" token
        seg_starts[p + 1] = tokens + pipe_positions[p] + 1;
    }

    // Create pipe fd pairs
    int pipefds[pipe_count][2];
    for (int i = 0; i < pipe_count; i++){
        if (pipe(pipefds[i]) == -1){
            perror("pipe");
            return;
        }
    }

    // Fork a child for each segment
    pid_t pids[num_segments];
    for (int i = 0; i < num_segments; i++){
        pids[i] = fork();
        if (pids[i] < 0){
            perror("fork");
            return;
        }
        if (pids[i] == 0){
            // If not the first segment, read stdin from previous pipe
            if (i > 0){
                dup2(pipefds[i - 1][0], STDIN_FILENO);
            }
            // If not the last segment, write stdout to current pipe
            if (i < pipe_count){
                dup2(pipefds[i][1], STDOUT_FILENO);
            }

            // Close all pipe fds in the child
            for (int p = 0; p < pipe_count; p++){
                close(pipefds[p][0]);
                close(pipefds[p][1]);
            }

            // Use get_cmd_tokens to extract redirects, then execute
            int *which_special = malloc(MAX_TOKENS * sizeof(int));
            int count = 0;
            char **cmd_tokens = malloc(sizeof(char *) * (MAX_TOKENS + 1));
            get_cmd_tokens(seg_starts[i], which_special, &count, cmd_tokens);

            setup_redirect_execute(seg_starts[i], which_special, cmd_tokens, count);
            free(cmd_tokens);
            free(which_special);
            exit(EXIT_FAILURE);
        }
    }

    // Parent: close all pipe fds
    for (int i = 0; i < pipe_count; i++){
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }

    // Wait for all children
    for (int i = 0; i < num_segments; i++){
        waitpid(pids[i], NULL, 0);
    }
}


// This is for a simple special command, like a single pipe or a single redirect
void special_command_run(char **tokens, int which_special){

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



// this is for handling multiple special commands in one go
void special_commands_run(char **tokens){
    int *which_special = malloc(MAX_TOKENS * sizeof(int));
    int count = 0, i, j;
    for (i = 0; tokens[i] != NULL; i++){
        for (j = 0; special_commands[j] != NULL; j++){
            if (strcmp(tokens[i], special_commands[j]) == 0){
                which_special[count++] = i;
                break;
            }
        }
    }
    which_special[count] = -1;
    handle_multiple_pipes(tokens);
    // if (count == 1){
    //     special_command_run(tokens, which_special[0]);
    // }
    // else{
    //     // to be continued after studying operator precedance
    //     // temporaryly just handle multiple redirects
    //     multiple_redirects_run(tokens, which_special, count);
    // }

    free(which_special);
    return;
}