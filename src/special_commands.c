#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"

// ---- Redirection helpers ----

/**
 * Sets up a file descriptor for redirection.
 * @param tokens Token array
 * @param redir_pos Index of the redirect operator
 * @return 0 on success, -1 on failure
 */
int setup_redirection_fd(char **tokens, int redir_pos){
    int fd;
    int to_fd = -1;
    char *redirect = tokens[redir_pos];
    char *filename = tokens[redir_pos + 1];

    if (filename == NULL){
        fprintf(stderr, "syntax error: redirect '%s' missing filename\n", redirect);
        return -1;
    }

    int flags = O_WRONLY | O_CREAT;
    if (strcmp(redirect, ">>") == 0 || strcmp(redirect, "2>>") == 0){
        flags |= O_APPEND;
    } else if (strcmp(redirect, ">") == 0 || strcmp(redirect, "2>") == 0){
        flags |= O_TRUNC;
    }

    if (strcmp(redirect, "<") == 0){
        to_fd = STDIN_FILENO;
        fd = open(filename, O_RDONLY);
    } else if (strcmp(redirect, ">") == 0 || strcmp(redirect, ">>") == 0){
        to_fd = STDOUT_FILENO;
        fd = open(filename, flags, 0644);
    } else if (strcmp(redirect, "2>") == 0 || strcmp(redirect, "2>>") == 0){
        to_fd = STDERR_FILENO;
        fd = open(filename, flags, 0644);
    } else {
        fprintf(stderr, "Invalid redirect operator: %s\n", redirect);
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

/**
 * Saves standard input, output, and error file descriptors.
 * @param saved_fds Array to store the 3 saved fds
 * @return 0 on success, -1 on failure
 */
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

/**
 * Restores standard file descriptors from a saved array.
 * @param saved_fds Array containing the 3 saved fds
 * @return 0 on success, -1 on failure
 */
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

/**
 * Sets up redirections and executes the command.
 * @param tokens Token array
 * @param redir_positions Array of redirect operator indices
 * @param redir_count Number of redirections
 * @param cmd_tokens Command tokens to execute
 * @return 0 on success, -1 on failure
 */
int setup_redirect_execute(char **tokens, int *redir_positions, int redir_count, char **cmd_tokens){
    for (int i = 0; i < redir_count; i++){
        if (setup_redirection_fd(tokens, redir_positions[i]) < 0){
            fprintf(stderr, "Redirection setup failed: %s\n", tokens[redir_positions[i]]);
            return -1;
        }
    }
    if (cmd_tokens[0] != NULL){
        exec_standard(cmd_tokens);
    }
    return 0;
}

/**
 * Extracts tokens into 2 categories: redirections and command tokens for command execution.
 * @param tokens Token array
 * @param ops Pre-scanned operators
 * @param start Start index of segment
 * @param end End index of segment
 * @param redir_positions Output array for redirect indices
 * @param redir_count Output count of redirections
 * @param cmd_tokens Output array for command tokens
 */
void get_cmd_tokens(char **tokens, ops_t *ops, int start, int end,
                    int *redir_positions, int *redir_count, char **cmd_tokens){
    *redir_count = 0;

    // Collect redirects in [start, end)
    for (int i = 0; i < ops->redir_count; i++){
        if (ops->redir_pos[i] >= start && ops->redir_pos[i] < end){
            redir_positions[(*redir_count)++] = ops->redir_pos[i];
        }
    }

    // Build cmd_tokens: tokens in [start, end) that are NOT redirect ops or their filenames
    int j = 0;
    int r = 0; // index into redir_positions
    for (int i = start; i < end; i++){
        if (r < *redir_count && i == redir_positions[r]){
            i++; // skip the redirect operator AND the filename (i++ here, loop i++ skips filename)
            r++;
            continue;
        }
        cmd_tokens[j++] = tokens[i];
    }
    cmd_tokens[j] = NULL;
}


// ---- Layer 4: Handle redirections for a single command segment ----

/**
 * Handles multiple redirections for a single command segment.
 * @param tokens Token array
 * @param ops Pre-scanned operators
 * @param start Start index of segment
 * @param end End index of segment
 * @return Wait status of the child, or -1 on fork failure
 */
int multiple_redirects_run(char **tokens, ops_t *ops, int start, int end){
    int redir_positions[MAX_TOKENS_LIMIT];
    int redir_count = 0;
    char *cmd_tokens[MAX_TOKENS_LIMIT + 1];

    get_cmd_tokens(tokens, ops, start, end, redir_positions, &redir_count, cmd_tokens);

    // Apply alias expansion BEFORE the builtin/fork decision.
    // This ensures aliased builtins (e.g. "alias back cd ..") are correctly
    // identified as parent-process builtins rather than being forked.
    if (cmd_tokens[0] != NULL){
        if (apply_aliases(cmd_tokens) != 0){
            return -1;
        }
    }

    int status = 0;

    if (cmd_tokens[0] == NULL){
        // No command, only redirects (e.g. "> file")
        pid_t pid = fork();
        if (pid < 0){
            perror("fork");
            return -1;
        } else if (pid == 0){
            setup_redirect_execute(tokens, redir_positions, redir_count, cmd_tokens);
            _exit(0);
        } else {
            wait(&status);
        }
    } else if (strcmp(cmd_tokens[0], "exit") == 0 || strcmp(cmd_tokens[0], "cd") == 0 || strcmp(cmd_tokens[0], "alias") == 0){
        // Built-in: run in parent with saved/restored fds
        int fds[3];
        if (save_fds(fds) < 0) return -1;
        int ret = setup_redirect_execute(tokens, redir_positions, redir_count, cmd_tokens);
        if (restore_fds(fds) < 0) return -1;
        if (ret < 0) status = -1;
    } else {
        // External command: fork and exec
        pid_t pid = fork();
        if (pid < 0){
            perror("fork");
            return -1;
        } else if (pid == 0){
            if (setup_redirect_execute(tokens, redir_positions, redir_count, cmd_tokens) < 0){
                _exit(EXIT_FAILURE);
            }
            _exit(0);
        } else {
            wait(&status);
        }
    }
    return status;
}

// ---- Layer 3: Handle pipe operators within a range ----

/**
 * Handles multiple pipe operators within a range.
 * @param tokens Token array
 * @param ops Pre-scanned operators
 * @param start Start index of segment
 * @param end End index of segment
 * @return Wait status of the last command in pipeline, or -1 on error
 */
int handle_multiple_pipes(char **tokens, ops_t *ops, int start, int end){
    // Collect pipe positions within [start, end)
    int local_pipes[MAX_TOKENS_LIMIT];
    int pipe_count = 0;
    for (int i = 0; i < ops->pipe_count; i++){
        if (ops->pipe_pos[i] >= start && ops->pipe_pos[i] < end){
            local_pipes[pipe_count++] = ops->pipe_pos[i];
        }
    }

    // If no pipes, just run with redirects
    if (pipe_count == 0){
        return multiple_redirects_run(tokens, ops, start, end);
    }

    int num_segments = pipe_count + 1;

    // Build segment boundaries: [seg_start[i], seg_end[i])
    int seg_start[num_segments];
    int seg_end[num_segments];
    seg_start[0] = start;
    for (int p = 0; p < pipe_count; p++){
        seg_end[p] = local_pipes[p];       // segment ends at the pipe
        seg_start[p + 1] = local_pipes[p] + 1; // next segment starts after the pipe
    }
    seg_end[num_segments - 1] = end;

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
            for (int j = 0; j < pipe_count; j++){
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }
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

            // Extract redirects and command for this segment, then execute
            int redir_positions[MAX_TOKENS_LIMIT];
            int redir_count = 0;
            char *cmd_tokens[MAX_TOKENS_LIMIT + 1];
            get_cmd_tokens(tokens, ops, seg_start[i], seg_end[i],
                           redir_positions, &redir_count, cmd_tokens);

            setup_redirect_execute(tokens, redir_positions, redir_count, cmd_tokens);
            _exit(EXIT_FAILURE);
        }
    }

    // Parent: close all pipe fds
    for (int i = 0; i < pipe_count; i++){
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }

    // Wait for all children, capture status of the last one
    int status = 0;
    for (int i = 0; i < num_segments; i++){
        waitpid(pids[i], &status, 0);
    }
    return status;
}

// ---- Layer 2: Handle && and || operators within a range ----

/**
 * Handles && and || operators within a range.
 * @param tokens Token array
 * @param ops Pre-scanned operators
 * @param start Start index
 * @param end End index
 * @return Status of the last executed command
 */
int handle_and_or(char **tokens, ops_t *ops, int start, int end){
    // Collect && / || positions within [start, end)
    int local_pos[MAX_TOKENS_LIMIT];
    int local_types[MAX_TOKENS_LIMIT];
    int local_count = 0;
    for (int i = 0; i < ops->andor_count; i++){
        if (ops->andor_pos[i] >= start && ops->andor_pos[i] < end){
            local_pos[local_count] = ops->andor_pos[i];
            local_types[local_count] = ops->andor_types[i];
            local_count++;
        }
    }

    // If no && or ||, just run with pipes
    if (local_count == 0){
        return handle_multiple_pipes(tokens, ops, start, end);
    }

    int num_segments = local_count + 1;

    // Build segment boundaries
    int seg_start[num_segments];
    int seg_end[num_segments];
    seg_start[0] = start;
    for (int p = 0; p < local_count; p++){
        seg_end[p] = local_pos[p];
        seg_start[p + 1] = local_pos[p] + 1;
    }
    seg_end[num_segments - 1] = end;

    // Run segments left-to-right, short-circuiting based on operator
    int status = handle_multiple_pipes(tokens, ops, seg_start[0], seg_end[0]);
    for (int i = 0; i < local_count; i++){
        if (local_types[i] == 0){ // &&
            if (status != 0) continue;
        } else { // ||
            if (status == 0) continue;
        }
        status = handle_multiple_pipes(tokens, ops, seg_start[i + 1], seg_end[i + 1]);
    }
    return status;
}

// ---- Layer 1: Handle ; and & operators (top-level) ----

/**
 * Handles ; and & top-level operators.
 * @param tokens Token array
 * @param ops Pre-scanned operators
 * @param start Start index
 * @param end End index
 * @return Status of the last foreground command
 */
int special_commands_run(char **tokens, ops_t *ops, int start, int end){
    // Collect ; / & positions within [start, end)
    // no need for it in the current version, but will be useful if we add 
    // higher precedance special operators
    int local_pos[MAX_TOKENS_LIMIT];
    int local_types[MAX_TOKENS_LIMIT];
    int local_count = 0;
    for (int i = 0; i < ops->seqbg_count; i++){
        if (ops->seqbg_pos[i] >= start && ops->seqbg_pos[i] < end){
            local_pos[local_count] = ops->seqbg_pos[i];
            local_types[local_count] = ops->seqbg_types[i];
            local_count++;
        }
    }

    // If no ; or &, just run with and/or
    if (local_count == 0){
        return handle_and_or(tokens, ops, start, end);
    }

    int num_segments = local_count + 1;

    // Build segment boundaries
    int seg_start[num_segments];
    int seg_end[num_segments];
    seg_start[0] = start;
    for (int p = 0; p < local_count; p++){
        seg_end[p] = local_pos[p];
        seg_start[p + 1] = local_pos[p] + 1;
    }
    seg_end[num_segments - 1] = end;

    // Run each segment according to the operator that follows it
    int status = 0;
    for (int i = 0; i < num_segments; i++){
        // Skip empty segments (e.g. trailing ; or &)
        if (seg_start[i] >= seg_end[i]) continue;

        if (i < local_count && local_types[i] == 1){ // & : run in background
            pid_t pid = fork();
            if (pid < 0){
                perror("fork");
                return -1;
            }
            if (pid == 0){
                handle_and_or(tokens, ops, seg_start[i], seg_end[i]);
                _exit(0);
            }
            // parent does not wait — background
        } else { // ; or last segment: run in foreground
            status = handle_and_or(tokens, ops, seg_start[i], seg_end[i]);
        }
    }
    return status;
}