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
    if (tokens[1] == NULL){
        fprintf(stderr, "cd: expected argument\n");
        return;
    } else if (tokens[2] != NULL){
        fprintf(stderr, "cd: too many arguments\n");
        return;
    }
    
    
    // Try to change directory
    if (chdir(tokens[1]) != 0) {
        char error_msg[512];
        memset(error_msg, 0, sizeof(error_msg));
        snprintf(error_msg, sizeof(error_msg), "cd: %s", tokens[1]);
        perror(error_msg);
    }
}


// used in forked children (caller forks and call this function)
void exec_standard(char **tokens){
    if (strcmp(tokens[0], "exit")==0){
        exit(0);
    }
    if (strcmp(tokens[0], "cd")==0){
        cd_handler(tokens);
        return;
    }

    execvp(tokens[0], tokens);
    // If execvp returns, there was an error
    perror(tokens[0]);
    exit(EXIT_FAILURE);
}


// used when no need to fork in the caller (caller does not fork)
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
    } 
    
    // Parent process
    wait(NULL);

    return;
}