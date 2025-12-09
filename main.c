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
}

void display_promt(){
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    char *username= getenv("LOGNAME");
    char cwd[256];
    getcwd(cwd, 256);
    printf("%s@%s:%s$ ", username, hostname, cwd);
}
// ------------------------------
// Test main
// int main(){
//     printf("\033[2J\033[H");
// }
// ------------------------------


int main(){
    char command[256];
    setvbuf(stdout, NULL, _IONBF, 0);
    while (true)
    {
        display_promt();
        if (fgets(command, sizeof(command), stdin) == NULL) {
            perror("Error reading command");
            continue;
        }
        execute(command);
    }
    
    return 0;
}