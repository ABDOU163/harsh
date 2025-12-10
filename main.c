#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <unistd.h>
#include <sys/types.h>


void execute(char *command){
    bool is_special = false;
    int which_special = -1;
    char **tokens = tokenize_with_distinction(command, &is_special, &which_special);
    if (is_special == false){
        standard_command_run(tokens);
    } else{
        // Handle special commands
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
int test_main(){
    
      
    return 0;
}
// ------------------------------


int main(int argc, char *argv[], char *envp[]){
    return real_main();
}