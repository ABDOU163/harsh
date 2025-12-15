#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

void cd_handler(char **tokens){
    char *path;
    
    if (tokens[1] == NULL){
        fprintf(stderr, "cd: expected argument\n");
        return;
    } else if (tokens[2] != NULL){
        fprintf(stderr, "cd: too many arguments\n");
        return;
    }
    
    if (strcmp(tokens[1], "~") == 0){
        path = getenv("HOME");
        if (path == NULL) {
            fprintf(stderr, "cd: HOME environment variable not set\n");
            return;
        }
    } else {
        path = tokens[1];
    }
    
    // Try to change directory
    if (chdir(path) != 0) {
        char error_msg[512];
        memset(error_msg, 0, sizeof(error_msg));
        snprintf(error_msg, sizeof(error_msg), "cd: %s", path);
        perror(error_msg);
    }
}

void standard_command_run(char **tokens){
    if (strcmp(tokens[0], "exit")==0){
        exit(0);
    }
    if (strcmp(tokens[0], "cd")==0){
        cd_handler(tokens);
        return;
    }
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        execvp(tokens[0], tokens);
        // If execvp returns, there was an error
        perror(tokens[0]);
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        wait(NULL);

    }
    return;
}