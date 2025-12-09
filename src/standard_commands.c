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
    } else if (tokens[2] != NULL){
        fprintf(stderr, "cd: too many arguments\n");
    }
    if (!strcmp(tokens[1], "~")){
        chdir(getenv("HOME"));
    }else{
        chdir(tokens[1]);
    }
}

void standard_command_run(char **tokens){
    if (!strcmp(tokens[0], "exit")){
        exit(0);
    }
    if (!strcmp(tokens[0], "cd")){
        cd_handler(tokens);
        return;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execvp(tokens[0], tokens);
    } else {
        // Parent process
        wait(NULL);
    }
}

