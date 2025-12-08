#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void standard_command_run(char **tokens){
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execvp(tokens[0], tokens);
    } else {
        // Parent process
        wait(NULL); // Wait for child process to finish
    }
    free(tokens);
}

