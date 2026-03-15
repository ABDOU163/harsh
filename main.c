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

bool shell_should_exit = false;
int shell_exit_status = 0;

/**
 * Perform history expansion on a command string.
 * Handles !!, !n, !-n, !string. Must be called BEFORE add_history.
 * @param command Raw command string from user input
 * @return Heap-allocated string to execute (caller must free), or NULL if
 *         the command should not be executed (error or display-only)
 */
char* expand_history(const char *command){
    char *expanded = NULL;
    int ret = history_expand((char*)command, &expanded);
    if (ret == 2){
        printf("%s\n", expanded);
        free(expanded);
        return NULL;
    }
    if (ret < 0){
        fprintf(stderr, "%s\n", expanded);
        free(expanded);
        return NULL;
    }
    if (ret == 0){
        free(expanded);
        return strdup(command);
    }
    // ret == 1: expansion happened, return the expanded string
    return expanded;
}


/**
 * Tokenize and execute a shell command line.
 * Pipeline: history expansion → add to history → tokenize → scan → dispatch.
 * @param command Raw command string from user input (not modified)
 */
void execute(char *command){
    char *to_run = expand_history(command);
    if (to_run == NULL) return;

    add_history(to_run);

    char **tokens = tokenize(to_run);
    free(to_run);
    if (tokens == NULL) return;

    int total = 0;
    while (tokens[total] != NULL) total++;
    ops_t ops;
    scan_operators(tokens, &ops);
    command_run(tokens, &ops, 0, total);
    free_tokens(tokens);
}

/**
 * Build the shell prompt string.
 * Format: username@hostname:cwd$ with ~ substitution for the home directory.
 * @return Heap-allocated prompt string (caller must free), or NULL on failure
 */
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


/**
 * Main shell REPL loop.
 * Reads user input via readline, manages history, and dispatches commands.
 * Runs until EOF (Ctrl+D) is received.
 * @return 0 on normal exit
 */
int real_main(){
    setvbuf(stdout, NULL, _IONBF, 0);
    if (init_dirstack() != 0){
        fprintf(stderr, "Failed to initialize directory stack\n");
        return 1;
    }
    if (init_alias_table() != 0){
        fprintf(stderr, "Failed to initialize alias table\n");
        return 1;
    }
    stifle_history(HISTORY_LENGTH);
    while (true)
    {
        char *prompt = build_prompt();
        if (prompt == NULL){
            perror("Failed to build prompt");
            free(prompt);
            continue;
        }
        char *line = readline(prompt);
        free(prompt);
        if (line == NULL){
            // EOF (Ctrl+D)
            printf("\n");
            free(line);
            break;
        }
        if (*line == '\0'){
            free(line);
            continue;
        }
        execute(line);
        free(line);
        if (shell_should_exit) {
            break;
        }
    }
    // Cleanup readline internals
    rl_clear_history();
    rl_free_line_state();
    rl_cleanup_after_signal();
    free_alias_table();
    free_dirstack();
    return shell_exit_status;
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