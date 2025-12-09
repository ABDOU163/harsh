#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <unistd.h>
#include <sys/types.h>
// #include <sys/wait.h>

void standard_command_run(char **tokens){
    pid_t pid = fork();
    int status, options;
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        if (!strcmp(tokens[0], "exit")) {
            exit(0);
        }
        if (!strcmp(tokens[0], "cd")){

        }
        execvp(tokens[0], tokens);
    } else {
        // Parent process
        // options = ;
        waitpid(pid, &status, options);
        // it depends on status of the child
        if (status){
            ;
        }
    }
    free(tokens);
}

