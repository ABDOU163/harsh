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
    char **tokens = tokenize(command);
    if (tokens == NULL){
        return;
    }
    if (special_commands_run(tokens) < 0){
        free_tokens(tokens);
        fprintf(stderr, "Command run failed\n");
        return;
    }
    free_tokens(tokens);
}

char* build_prompt(){
    char prompt[512];
    char hostname[128];
    gethostname(hostname, sizeof(hostname));
    char *username = getenv("LOGNAME");
    if (!username) username = "unknown";
    char cwd[128];
    if (getcwd(cwd, sizeof(cwd)) == NULL){
        perror("getcwd");
        strcpy(cwd, "?");
    }

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
        if (prompt == NULL){
            perror("Failed to build prompt");
            continue;
        }
        char *line = readline(prompt);
        if (line == NULL){
            // EOF (Ctrl+D)
            free(prompt);
            printf("\n");
            break;
        }
        if (*line == '\0'){
            free(prompt);
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
    sa.sa_flags = SA_RESTART | SA_NOCLDWAIT;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    return real_main();
}