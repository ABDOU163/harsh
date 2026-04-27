#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

// ---- Directory stack ----

#define INIT_DIRSTACK_CAPACITY 8

typedef struct {
    char **dirs;
    int count;
    int capacity;
} dirstack_t;

dirstack_t dirstack = {0};

/**
 * Initialize the directory stack.
 * @return 0 on success, -1 on allocation failure
 */
int init_dirstack(){
    dirstack.capacity = INIT_DIRSTACK_CAPACITY;
    dirstack.count = 0;
    dirstack.dirs = malloc(dirstack.capacity * sizeof(char*));
    dirstack.dirs[0] = NULL;
    if (dirstack.dirs == NULL){
        perror("malloc: dirstack");
        return -1;
    }
    return 0;
}

void free_dirstack(){
    for (int i = 0; i < dirstack.count; i++)
        free(dirstack.dirs[i]);
    free(dirstack.dirs);
    dirstack.dirs = NULL;
    dirstack.count = 0;
    dirstack.capacity = 0;
}

/**
 * Push a directory onto the stack.
 * @param dir Directory path (will be strdup'd)
 * @return 0 on success, -1 on allocation failure
 */
static int dirstack_push(const char *dir){
    if (dirstack.count >= dirstack.capacity){
        int new_cap = dirstack.capacity * 2;
        char **new_dirs = realloc(dirstack.dirs, new_cap * sizeof(char*));
        if (new_dirs == NULL){
            perror("realloc: dirstack");
            return -1;
        }
        dirstack.dirs = new_dirs;
        dirstack.capacity = new_cap;
    }
    dirstack.dirs[dirstack.count] = strdup(dir);
    if (dirstack.dirs[dirstack.count] == NULL){
        perror("strdup: dirstack");
        return -1;
    }
    dirstack.count++;
    return 0;
}

/**
 * Pop a directory from the stack.
 * @return Heap-allocated directory string (caller must free), or NULL if empty
 */
static char* dirstack_pop(){
    if (dirstack.count == 0) return NULL;
    dirstack.count--;
    return dirstack.dirs[dirstack.count];
}

// ---- Builtins ----

/**
 * Handle the built-in cd command.
 * @param tokens NULL-terminated token array where tokens[0] is "cd"
 * @return 0 on success, 1 on error (missing arg, too many args, chdir failure)
 */
int cd_handler(char **tokens){
    if (tokens[1] == NULL){
        fprintf(stderr, "cd: expected argument\n");
        return -1;
    } else if (tokens[2] != NULL){
        fprintf(stderr, "cd: too many arguments\n");
        return -1;
    }
    
    // Try to change directory
    if (chdir(tokens[1]) != 0) {
        char error_msg[512];
        memset(error_msg, 0, sizeof(error_msg));
        snprintf(error_msg, sizeof(error_msg), "cd: %s", tokens[1]);
        perror(error_msg);
        return -1;
    }
    return 0;
}

/**
 * Handle the pushd builtin: push current directory onto the stack, then cd.
 * Usage: pushd <dir>
 * @param tokens NULL-terminated token array where tokens[0] is "pushd"
 * @return 0 on success, 1 on error
 */
int pushd_handler(char **tokens){
    if (tokens[1] == NULL){
        fprintf(stderr, "pushd: expected argument\n");
        return -1;
    }

    char cwd[512];
    if (getcwd(cwd, sizeof(cwd)) == NULL){
        perror("pushd: getcwd");
        return -1;
    }

    // Try cd first, only push if it succeeds
    if (cd_handler(tokens) != 0){
        return -1;
    }

    if (dirstack_push(cwd) != 0){
        fprintf(stderr, "pushd: failed to save directory\n");
        return -1;
    }

    return 0;
}

/**
 * Handle the popd builtin: pop directory from stack and cd to it.
 * Usage: popd
 * @param tokens NULL-terminated token array where tokens[0] is "popd"
 * @return 0 on success, 1 on error (empty stack, chdir failure)
 */
int popd_handler(char **tokens){
    char *dir = dirstack_pop();
    if (dir == NULL){
        fprintf(stderr, "popd: directory stack empty\n");
        return -1;
    }

    if (chdir(dir) != 0){
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "popd: %s", dir);
        perror(error_msg);
        free(dir);
        return -1;
    }

    free(dir);
    dirstack.dirs[dirstack.count] = NULL;
    return 0;
}

// ---- Command dispatch ----

/**
 * Check if a command is a shell builtin that must run in the parent process.
 * @param cmd Command name (tokens[0])
 * @return true if cmd is a builtin
 */
bool is_builtin(const char *cmd){
    return strcmp(cmd, "cd") == 0 ||
           strcmp(cmd, "exit") == 0 ||
           strcmp(cmd, "alias") == 0 ||
           strcmp(cmd, "unalias") == 0 ||
           strcmp(cmd, "pushd") == 0 ||
           strcmp(cmd, "popd") == 0;
}

/**
 * Execute a builtin command in the parent shell process.
 * Must only be called for commands where is_builtin() returns true.
 * @param tokens NULL-terminated token array where tokens[0] is the builtin
 * @return 0 on success, non-zero on error (exit never returns)
 */
int exec_builtin(char **tokens){
    if (strcmp(tokens[0], "exit") == 0){
        shell_should_exit = true;
        if (tokens[1] != NULL) {
            shell_exit_status = atoi(tokens[1]);
        }
        return 0;
    }
    if (strcmp(tokens[0], "cd") == 0){
        return cd_handler(tokens);
    }
    if (strcmp(tokens[0], "alias") == 0){
        return alias_command(tokens);
    }
    if (strcmp(tokens[0], "unalias") == 0){
        return unalias_command(tokens);
    }
    if (strcmp(tokens[0], "pushd") == 0){
        return pushd_handler(tokens);
    }
    if (strcmp(tokens[0], "popd") == 0){
        return popd_handler(tokens);
    }
    return -1;
}

/**
 * Execute an external command by replacing the process image with execvp.
 * On success, this function does not return (process image is replaced).
 * On failure (command not found), returns -1 so the caller can clean up
 * heap allocations before calling _exit.
 * @param tokens NULL-terminated token array where tokens[0] is the command
 * @return -1 on execvp failure (never returns on success)
 */
int exec_external(char **tokens){
    execvp(tokens[0], tokens);
    perror(tokens[0]);
    return -1;
}

/**
 * Dispatch a command: builtins run in-process, externals via execvp.
 * @param tokens NULL-terminated token array where tokens[0] is the command
 * @return builtin return code, or -1 if external command fails
 */
int exec_standard(char **tokens){
    char **globbed = glob_expansion(tokens);
    int res = is_builtin(globbed[0]) ? exec_builtin(globbed) : exec_external(globbed);
    free_tokens(globbed);
    return res;
}