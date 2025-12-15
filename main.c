#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>

void execute(char *command){
    bool is_special = false;
    char **tokens = tokenize(command, &is_special);
    if (is_special == false){
        standard_command_run(tokens);
    } else{
        // to change later
        special_commands_run(tokens);
    }
    free(tokens);
}

void display_prompt(){
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    char *username= getenv("LOGNAME");
    char cwd[256];
    getcwd(cwd, 256);

    // checking so /home/username = ~
    char *temp=cwd;
    char *home = getenv("HOME");
    int home_len = strlen(home);
    bool is_home = true;
    int i=0;
    while (is_home && i<home_len){
        if (cwd[i] != home[i]){
            is_home = false;
        } else {
            i++;
        }
    }
    if (is_home){
        temp+= i-1;
        *temp = '~';
    }

    printf("%s@%s:%s$ ", username, hostname, temp);
}


int real_main(){
    char command[256];
    setvbuf(stdout, NULL, _IONBF, 0);
    while (true)
    {
        display_prompt();
        if (fgets(command, sizeof(command), stdin) == NULL) {
            perror("Error reading command");
            continue;
        }
        execute(command);
    }
    
    return 0;
}


// ------------------------------
// Test main
void test_main(){
    int fd[2];
    pipe(fd);  // fd[0] = read end, fd[1] = write end

    if (fork() == 0) {
        // child 1: producer
        dup2(fd[1], 1);   // stdout -> pipe write end
        close(fd[0]);
        close(fd[1]);

        execlp("ls", "ls", NULL);
    }

    if (fork() == 0) {
        // child 2: consumer
        dup2(fd[0], 0);   // stdin -> pipe read end
        close(fd[1]);
        close(fd[0]);

        execlp("grep", "grep", "src", NULL);
    }

    // parent
    close(fd[0]);
    close(fd[1]);

    wait(NULL);
    wait(NULL);

    return ;
}
// ------------------------------


int main(int argc, char *argv[], char *envp[]){
    real_main();
    return 0;
}