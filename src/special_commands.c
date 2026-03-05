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
int setup_redirect_execute(char **tokens, int *which_special, char **cmd_tokens, int count){
    for (int i=0; i < count; i++){
        if (setup_redirection_fd(tokens, which_special[i]) < 0){
            fprintf(stderr, "Redirection setup failed: %s %s\n", tokens[which_special[i]], tokens[which_special[i] + 1]);
            return -1;
        }
    }
    if (*cmd_tokens != NULL){
        exec_standard(cmd_tokens);
    }
    return 0;
}

int save_fds(int *saved_fds){
    saved_fds[0] = dup(STDIN_FILENO);
    saved_fds[1] = dup(STDOUT_FILENO);
    saved_fds[2] = dup(STDERR_FILENO);
    if (saved_fds[0] < 0 || saved_fds[1] < 0 || saved_fds[2] < 0){
        perror("Failed to save file descriptors");
        return -1;
    }
    return 0;
}

int restore_fds(int *saved_fds){
    if (dup2(saved_fds[0], STDIN_FILENO) < 0 || dup2(saved_fds[1], STDOUT_FILENO) < 0 || dup2(saved_fds[2], STDERR_FILENO) < 0){
        perror("Failed to restore file descriptors");
        return -1;
    }
    if (close(saved_fds[0]) < 0 || close(saved_fds[1]) < 0 || close(saved_fds[2]) < 0){
        perror("Failed to close file descriptors");
        return -1;
    }
    return 0;
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
int multiple_redirects_run(char **tokens){
    int *which_special = malloc(MAX_TOKENS * sizeof(int));
    if (which_special == NULL){
        return -1;
    }
    int count = 0;
    int status = 0;
    char **cmd_tokens = malloc(sizeof(char *) * (MAX_TOKENS + 1));
    if (cmd_tokens == NULL){
        free(which_special);
        return -1;
    }
    get_cmd_tokens(tokens, which_special, &count, cmd_tokens);

    if (*cmd_tokens == NULL){
        // No command, only redirects (e.g. "> file")
        // Still set up redirects in case there are side effects (file creation)
        pid_t pid = fork();
        if (pid < 0){
            perror("fork");
            status = -1;
        } else if (pid == 0){
            setup_redirect_execute(tokens, which_special, cmd_tokens, count);
            _exit(0);
        } else {
            wait(&status);
        }
    } else if (strcmp(*cmd_tokens, "exit") == 0 || strcmp(*cmd_tokens, "cd") ==0){
        int fds[3];
        if (save_fds(fds) < 0){
            _exit(EXIT_FAILURE);
        }
        int ret = setup_redirect_execute(tokens, which_special, cmd_tokens, count);
        if (restore_fds(fds) < 0){
            _exit(EXIT_FAILURE);
        }
        if (ret < 0){
            status = -1;
        }
    } else{
        pid_t pid = fork();
        if (pid < 0){
            perror("fork");
            status = -1;
        } else if (pid == 0){
            if (setup_redirect_execute(tokens, which_special, cmd_tokens, count) < 0){
                _exit(EXIT_FAILURE);
            }
        } else {
            wait(&status);
        }
    }
    free(cmd_tokens);
    free(which_special);
    return status;
}



int handle_multiple_pipes(char **tokens){
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
        return multiple_redirects_run(tokens);
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
            for (int j = 0; j < i; j++){
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }
            return -1;
        }
    }

    // Fork a child for each segment
    pid_t pids[num_segments];
    for (int i = 0; i < num_segments; i++){
        pids[i] = fork();
        if (pids[i] < 0){
            perror("fork");
            // Close all pipe fds so children get EOF/SIGPIPE and can terminate
            for (int j = 0; j < pipe_count; j++){
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }
            // Wait for already-forked children to prevent orphans
            for (int j = 0; j < i; j++){
                waitpid(pids[j], NULL, 0);
            }
            return -1;
        }
        if (pids[i] == 0){
            // If not the first segment, read stdin from previous pipe
            if (i > 0){
                if (dup2(pipefds[i - 1][0], STDIN_FILENO) < 0){
                    perror("dup2 stdin");
                    _exit(EXIT_FAILURE);
                }
            }
            // If not the last segment, write stdout to current pipe
            if (i < pipe_count){
                if (dup2(pipefds[i][1], STDOUT_FILENO) < 0){
                    perror("dup2 stdout");
                    _exit(EXIT_FAILURE);
                }
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
            if (!which_special || !cmd_tokens){
                perror("malloc");
                free(which_special);
                free(cmd_tokens);
                _exit(EXIT_FAILURE);
            }
            get_cmd_tokens(seg_starts[i], which_special, &count, cmd_tokens);

            if (setup_redirect_execute(seg_starts[i], which_special, cmd_tokens, count) <0){
                free(cmd_tokens);
                free(which_special);
                _exit(EXIT_FAILURE);
            }
            free(cmd_tokens);
            free(which_special);
        }
    }

    // Parent: close all pipe fds
    for (int i = 0; i < pipe_count; i++){
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }

    // Wait for all children, capture status of the last one
    int status = 0;
    int temp = status;
    for (int i = 0; i < num_segments; i++){
        waitpid(pids[i], &status, 0);
        temp |= status;
    }
    return temp;
}


int handle_and_or(char **tokens){
    // Collect positions and types of && and || operators
    int op_positions[MAX_TOKENS];
    int op_types[MAX_TOKENS]; // 0 = &&, 1 = ||
    int op_count = 0;
    for (int i = 0; tokens[i] != NULL; i++){
        if (strcmp(tokens[i], "&&") == 0){
            op_positions[op_count] = i;
            op_types[op_count] = 0;
            op_count++;
        } else if (strcmp(tokens[i], "||") == 0){
            op_positions[op_count] = i;
            op_types[op_count] = 1;
            op_count++;
        }
    }

    // If no && or ||, just run with handle_multiple_pipes
    if (op_count == 0){
        return handle_multiple_pipes(tokens);
    }

    int num_segments = op_count + 1;

    // Nullify operator tokens in-place and collect segment start indices
    int seg_starts[num_segments];
    seg_starts[0] = 0;
    for (int p = 0; p < op_count; p++){
        tokens[op_positions[p]] = NULL;
        seg_starts[p + 1] = op_positions[p] + 1;
    }

    // Run segments left-to-right, short-circuiting based on operator
    int status = handle_multiple_pipes(tokens);
    for (int i = 0; i < op_count; i++){
        if (op_types[i] == 0){ // &&
            if (status != 0) continue;
        } else { // ||
            if (status == 0) continue;
        }
        status = handle_multiple_pipes(tokens + seg_starts[i + 1]);
    }
    return 0;
}


int special_commands_run(char **tokens){
    // Collect positions and types of ; and & operators
    int op_positions[MAX_TOKENS];
    int op_types[MAX_TOKENS]; // 0 = ;, 1 = &
    int op_count = 0;
    for (int i = 0; tokens[i] != NULL; i++){
        if (strcmp(tokens[i], ";") == 0){
            op_positions[op_count] = i;
            op_types[op_count] = 0;
            op_count++;
        } else if (strcmp(tokens[i], "&") == 0){
            op_positions[op_count] = i;
            op_types[op_count] = 1;
            op_count++;
        }
    }

    // If no ; or &, just run with handle_and_or
    if (op_count == 0){
        return handle_and_or(tokens);
    }

    int num_segments = op_count + 1;

    // Nullify operator tokens in-place and collect segment start indices
    int seg_starts[num_segments];
    seg_starts[0] = 0;
    for (int p = 0; p < op_count; p++){
        tokens[op_positions[p]] = NULL;
        seg_starts[p + 1] = op_positions[p] + 1;
    }

    // Run each segment according to the operator that follows it
    int status = 0;
    for (int i = 0; i < num_segments; i++){
        // Skip empty segments (e.g. trailing ; or &)
        if (tokens[seg_starts[i]] == NULL) continue;

        if (i < op_count && op_types[i] == 1){ // & : run in background
            pid_t pid = fork();
            if (pid < 0){
                perror("fork");
                return -1;
            }
            if (pid == 0){
                handle_and_or(tokens + seg_starts[i]);
                _exit(0);
            }
            // parent does not wait — background
        } else { // ; or last segment: run in foreground
            status = handle_and_or(tokens + seg_starts[i]);
        }
    }
    return status;
}