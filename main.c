#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <glob.h>
#include <readline/readline.h>
#include <readline/history.h>

void execute(char *command){
    bool is_special = false;
    char **tokens = tokenize(command, &is_special);
    if (tokens[0] == NULL){
        free_tokens(tokens);
        return;
    }
    special_commands_run(tokens);
    free_tokens(tokens);
}

char* build_prompt(){
    char prompt[512];
    char hostname[128];
    gethostname(hostname, sizeof(hostname));
    char *username= getenv("LOGNAME");
    char cwd[128];
    getcwd(cwd, 128);

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

    snprintf(prompt, 512, "%s@%s:%s$ ", username, hostname, temp);
    return strdup(prompt);
}


int real_main(){
    setvbuf(stdout, NULL, _IONBF, 0);
    stifle_history(HISTORY_LENGTH);
    while (true)
    {
        char *prompt = build_prompt();
        char *line = readline(prompt);
        if (line == NULL){
            // EOF (Ctrl+D)
            printf("\n");
            break;
        }
        if (*line == '\0'){
            free(line);
            continue;
        }
        add_history(line);
        execute(line);
        free(line);
        free(prompt);
    }
    
    return 0;
}

// -----------------------------------------
// Signal handler function to reap zombie processes
// not used in current version
void sigchld_handler(int signum)
{
    pid_t pid;
    int status;

    // Reap all zombie processes
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("Parent process reaped child process with PID %d.\n", pid);
    }
}


// ------------------------------
// Test main


int test_main() {
    char token[33] = "test~*";
    glob_t glob_result;
    int ret = glob(token, GLOB_TILDE | GLOB_MARK, NULL, &glob_result);
    printf("return is %d\n", ret==GLOB_NOMATCH);
    if (ret == 0) {
        for (size_t i = 0; i < glob_result.gl_pathc; i++) {
            printf("%s\n", glob_result.gl_pathv[i]);
        }
        globfree(&glob_result);
    }
    

    return 0;
}
// ------------------------------

// todo:
// - handle multiple special commands in one go
// add tab completion using wildcard expansion (glob) and readline library
// study and implement signal handling for background processes (SIGCHLD) to prevent zombie processes
// study and implement processes and threads for handling multiple commands and background processes
int main(int argc, char *argv[], char *envp[]){
    pid_t child_pid;
    struct sigaction sa;

    // Register the signal handler for SIGCHLD
    sa.sa_handler = SIG_IGN ;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART || SA_NOCLDWAIT;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    return real_main();
}