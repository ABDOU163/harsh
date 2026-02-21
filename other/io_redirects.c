
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "globals.h"
#include "tokenizer.h"
#include "standard.h"
#include "special.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
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
