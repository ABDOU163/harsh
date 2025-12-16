#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <unistd.h>
#include <sys/types.h>


void handle_pipe(char **left_cmd, char **right_cmd){
    int fd[2];/*fd[0] for read and fd[1] for write*/
    pid_t p;
    if (pipe(fd) == -1){
        perror("pipefd: ");
        return;
    }
    

    p=fork();
    if (p < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);

    } else if (p==0){
        dup2(fd[1], 1);

        close(fd[0]);
        close(fd[1]);

        exec_standard(left_cmd);
        // execvp(left_cmd[0], left_cmd);
        // perror(left_cmd[0]);
        // exit(EXIT_FAILURE);

    }

    p=fork();
    if (p < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);

    } else if (p==0){
        dup2(fd[0], 0);

        close(fd[0]);
        close(fd[1]);
        exec_standard(right_cmd);
        // execvp(right_cmd[0], right_cmd);
        // perror(right_cmd[0]);
        // exit(EXIT_FAILURE);
    }

    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
    return;


}

// This is for a simple special command, like a single pipe or a single redirect
void special_command_run(char **tokens, int which_special){
    
    int len = MAX_TOKENS/2+1;
    int i;
    char **left_cmd = malloc(sizeof(char*) * (len + 1));
    char **right_cmd = malloc(sizeof(char*) * (len + 1));

    for (i=0; i < which_special; i++){
        left_cmd[i] = tokens[i];
    }
    left_cmd[i] = (char*)NULL;

    for (i=which_special+1; tokens[i] != NULL; i++){
        right_cmd[i-which_special-1] = tokens[i];
    }
    right_cmd[i-which_special-1] = (char*)NULL;

    
    // special command function choosing
    if (strcmp(tokens[which_special], "|") == 0){
        handle_pipe(left_cmd, right_cmd);
    } else{
        // to be continued
    }

    free(right_cmd);
    free(left_cmd);
    return;
}



// this is for handling multiple special commands in one go
void special_commands_run(char **tokens){
    int *which_special = malloc(MAX_TOKENS * sizeof(int));
    int count=0, i,j;
    for (i=0; special_commands[i] != NULL; i++){
        for (j=i ; tokens[j] != NULL ; j++){
            if (strcmp(special_commands[i], tokens[j]) == 0){
                which_special[count] = j;
                count+=1;
            }
        }
    }
    if (count == 1){
        special_command_run(tokens, which_special[0]);
    } else {
        // to be continued
    }
    return;
    
}