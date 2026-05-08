#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <unistd.h>
#include <errno.h>

// ---- Alias manager (global) ----

alias_manager_t aliases = {0};

/**
 * Initialize the alias table with hardcoded defaults and load ~/.harshrc.
 * Order: hardcoded defaults → .harshrc aliases.
 * @return 0 on success, -1 on allocation failure
 */
int init_alias_table(){
    aliases.capacity = INIT_ALIAS_CAPACITY;
    aliases.count = 0;
    aliases.table = malloc(aliases.capacity * sizeof(alias_t));
    if (aliases.table == NULL){
        perror("malloc: alias table");
        return -1;
    }

    // Load config file
    if (load_harshrc() != 0){
        free_alias_table();
        return -1;
    }

    return 0;
}

/**
 * Free the alias table and every alias name/replacement it owns.
 */
void free_alias_table(){
    if (aliases.table == NULL){
        return;
    }
    for (int i = 0; i < aliases.count; i++){
        free(aliases.table[i].name);
        for (int j = 0; j < aliases.table[i].args_count; j++){
            free(aliases.table[i].args[j]);
        }
        free(aliases.table[i].args);
    }
    free(aliases.table);
    aliases.table = NULL;
    aliases.count = 0;
    aliases.capacity = 0;
}

// ---- Alias table management ----

/**
 * Add or update an alias in the alias table.
 * If an alias with the same name exists, it is replaced.
 * If the table is full, it is reallocated to double capacity.
 * @param name   Alias name (will be strdup'd)
 * @param args   NULL-terminated array of replacement args (each will be strdup'd)
 * @param count  Number of args (not counting NULL)
 * @return 0 on success, -1 on allocation failure
 */
int add_alias(const char *name, char **args, int count){
    // Check if alias already exists → update
    for (int i = 0; i < aliases.count; i++){
        if (strcmp(aliases.table[i].name, name) == 0){
            // Free old args
            for (int j = 0; j < aliases.table[i].args_count; j++){
                free(aliases.table[i].args[j]);
            }
            free(aliases.table[i].args);

            // Allocate new args
            aliases.table[i].args = malloc((count + 1) * sizeof(char*));
            if (aliases.table[i].args == NULL){
                perror("malloc: alias args");
                return -1;
            }
            for (int j = 0; j < count; j++){
                aliases.table[i].args[j] = strdup(args[j]);
                if (aliases.table[i].args[j] == NULL){
                    perror("strdup: alias arg");
                    return -1;
                }
            }
            aliases.table[i].args[count] = NULL;
            aliases.table[i].args_count = count;
            return 0;
        }
    }

    // Grow table if needed
    if (aliases.count >= aliases.capacity){
        int new_cap = aliases.capacity * 2;
        alias_t *new_table = realloc(aliases.table, new_cap * sizeof(alias_t));
        if (new_table == NULL){
            perror("realloc: alias table");
            return -1;
        }
        aliases.table = new_table;
        aliases.capacity = new_cap;
    }

    // Add new entry
    alias_t *entry = &aliases.table[aliases.count];
    entry->name = strdup(name);
    if (entry->name == NULL){
        perror("strdup: alias name");
        return -1;
    }
    entry->args = malloc((count + 1) * sizeof(char*));
    if (entry->args == NULL){
        perror("malloc: alias args");
        free(entry->name);
        return -1;
    }
    for (int i = 0; i < count; i++){
        entry->args[i] = strdup(args[i]);
        if (entry->args[i] == NULL){
            perror("strdup: alias arg");
            // Cleanup partial
            for (int j = 0; j < i; j++) free(entry->args[j]);
            free(entry->args);
            free(entry->name);
            return -1;
        }
    }
    entry->args[count] = NULL;
    entry->args_count = count;
    aliases.count++;
    return 0;
}

// ---- Alias expansion ----

/**
 * Replace one command-position token with an alias value.
 *
 * Example: alias ll = {"ls", "-la", NULL}
 *   Before: ["echo", "x", ";", "ll", "foo/", NULL]
 *   After:  ["echo", "x", ";", "ls", "-la", "foo/", NULL]
 *
 * The main token array owns its strings, so alias replacement words are
 * duplicated here instead of borrowing pointers from the alias table.
 *
 * @param tokens Token array (fixed size MAX_TOKENS_LIMIT + 1)
 * @param pos    Index of the token to replace
 * @param entry  Alias entry to inject
 * @return 0 on success, -1 if tokens would exceed MAX_TOKENS_LIMIT
 */
static int replace_with_alias(char **tokens, int pos, alias_t *entry){
    int total = 0;
    while (tokens[total] != NULL) total++;

    int alias_argc = entry->args_count;
    int extra = alias_argc - 1;
    if (total + extra > MAX_TOKENS_LIMIT){
        fprintf(stderr, "%s: too many arguments after alias expansion\n", tokens[pos]);
        return -1;
    }

    char **copies = NULL;
    if (alias_argc > 0){
        copies = malloc(alias_argc * sizeof(char*));
        if (copies == NULL){
            perror("malloc: alias expansion");
            return -1;
        }
        for (int i = 0; i < alias_argc; i++){
            copies[i] = strdup(entry->args[i]);
            if (copies[i] == NULL){
                perror("strdup: alias expansion");
                for (int j = 0; j < i; j++){
                    free(copies[j]);
                }
                free(copies);
                return -1;
            }
        }
    }

    free(tokens[pos]);
    if (alias_argc == 0){
        memmove(&tokens[pos], &tokens[pos + 1], sizeof(char*) * (total - pos));
        return 0;
    }

    if (extra != 0){
        memmove(&tokens[pos + alias_argc], &tokens[pos + 1],
                sizeof(char*) * (total - pos));
    }

    for (int i = 0; i < alias_argc; i++){
        tokens[pos + i] = copies[i];
    }
    free(copies);
    return 0;
}

/**
 * Find an alias by name in the global alias table.
 * @param name Alias name to look up
 * @return Alias table index, or -1 if not found
 */
static int find_alias(const char *name){
    for (int i = 0; i < aliases.count; i++){
        if (strcmp(name, aliases.table[i].name) == 0){
            return i;
        }
    }
    return -1;
}

/**
 * Check whether an alias was already expanded in the current expansion chain.
 * @param applied Alias indices already expanded
 * @param applied_count Number of valid entries in applied
 * @param alias_idx Alias index to search for
 * @return true if alias_idx was already applied
 */
static bool has_applied_alias(int *applied, int applied_count, int alias_idx){
    for (int i = 0; i < applied_count; i++){
        if (applied[i] == alias_idx){
            return true;
        }
    }
    return false;
}

/**
 * Test whether a token starts a new simple command.
 * @param token Token text
 * @return true for operators after which aliases may expand again
 */
static bool is_command_separator(const char *token){
    return strcmp(token, "|") == 0 ||
           strcmp(token, ";") == 0 ||
           strcmp(token, "&") == 0 ||
           strcmp(token, "&&") == 0 ||
           strcmp(token, "||") == 0;
}

/**
 * Test whether a token is a redirection operator.
 * @param token Token text
 * @return true if token is one of the supported redirection operators
 */
static bool is_redirect_operator(const char *token){
    for (int i = 0; redirects[i] != NULL; i++){
        if (strcmp(token, redirects[i]) == 0){
            return true;
        }
    }
    return false;
}

/**
 * Expand chained aliases at a single command-word position.
 * Stops if the next alias would repeat an alias already used in the chain.
 * @param tokens Main token array
 * @param pos Command-word index to expand
 * @return 0 on success, -1 on allocation/size failure
 */
static int expand_command_word(char **tokens, int pos){
    int applied[MAX_TOKENS_LIMIT];
    int applied_count = 0;

    while (tokens[pos] != NULL){
        int alias_idx = find_alias(tokens[pos]);
        if (alias_idx < 0){
            break;
        }
        if (has_applied_alias(applied, applied_count, alias_idx)){
            break;
        }
        if (applied_count >= MAX_TOKENS_LIMIT){
            fprintf(stderr, "%s: too many nested aliases\n", tokens[pos]);
            return -1;
        }

        applied[applied_count++] = alias_idx;
        if (replace_with_alias(tokens, pos, &aliases.table[alias_idx]) != 0){
            return -1;
        }
    }
    return 0;
}

/**
 * Expand aliases across a whole command line before operator scanning.
 * Only command-word positions are expanded, matching the important Bash rule:
 * arguments such as the "x" in "echo x" are not alias candidates.
 * @param tokens Main token array
 * @return 0 on success, -1 on expansion failure
 */
int apply_aliases(char **tokens){
    bool command_position = true;

    for (int i = 0; tokens[i] != NULL; ){
        if (is_command_separator(tokens[i])){
            command_position = true;
            i++;
            continue;
        }

        if (is_redirect_operator(tokens[i])){
            i++;
            if (tokens[i] != NULL){
                i++;
            }
            continue;
        }

        if (!command_position){
            i++;
            continue;
        }

        if (expand_command_word(tokens, i) != 0){
            return -1;
        }

        if (tokens[i] == NULL){
            break;
        }

        if (is_command_separator(tokens[i]) || is_redirect_operator(tokens[i])){
            continue;
        }

        command_position = false;
        i++;
    }

    return 0;
}

// ---- alias command (interactive + .harshrc) ----

/**
 * Parse one alias definition token of the form name="command [args...]".
 * @param token Mutable definition token
 * @param name Output alias name, heap-allocated on success
 * @param cmd Output command string, heap-allocated on success
 * @return 0 on success, -1 on invalid syntax or allocation failure
 */
int valid_alias_command(char *token, char **name, char **cmd){
    char *p=strchr(token, '=');
    if (!p || (p-token == 0) || (p[1] != '"') || (p[strlen(p)-1] != '"')){
        fprintf(stderr, "alias: usage: alias name=\"command [args...]\"\n");
        return -1;
    }
    token[strlen(token)-1]= '\0';
    *p='\0';
    *name = strdup(token);
    if (*name == NULL){
        perror("strdup: alias name");
        return -1;
    }
    *cmd = strdup(p+2);
    if (*cmd == NULL){
        perror("strdup: alias command");
        free(*name);
        *name = NULL;
        return -1;
    }
    return 0;
}

/**
 * Handle the alias builtin command.
 * Usage:
 *   alias              → print all aliases
 *   alias name="cmd ..." → define alias "name" as "cmd ..."
 *
 * @param tokens NULL-terminated token array where tokens[0] is "alias"
 * @return 0 on success, 1 on error
 */
int alias_command(char **tokens){
    if (tokens[0] == NULL){
        return 0;
    }
    // No args: print all aliases
    if (tokens[1] == NULL){
        for (int i = 0; i < aliases.count; i++){
            printf("alias %s=\"", aliases.table[i].name);
            for (int j = 0; j < aliases.table[i].args_count; j++){
                if (j > 0) printf(" ");
                printf("%s", aliases.table[i].args[j]);
            }
            printf("\"\n");
        }
        return 0;
    }
    for (int j=1; tokens[j] != NULL; j++){
        char *name;
        char *cmd;
        int count;
        if (valid_alias_command(tokens[j], &name, &cmd) != 0){
            continue;
        }
        char **cmd_args = tokenize(cmd);
        if (!cmd_args){
            free(name);
            free(cmd);
            continue;
        }
        for (count=0; cmd_args[count] != NULL; count++);
        if (add_alias(name, cmd_args, count) != 0){
            free_tokens(cmd_args);
            free(name);
            free(cmd);
            return -1;
        }
        free_tokens(cmd_args);
        free(name);
        free(cmd);
    }
    return 0;
}

/**
 * Handle the unalias builtin command.
 * Usage: unalias name [name ...]
 * @param tokens NULL-terminated token array where tokens[0] is "unalias"
 * @return 0 on success, 1 on error
 */
int unalias_command(char **tokens){
    if (tokens[1] == NULL){
        fprintf(stderr, "unalias: usage: unalias [name ...]\n");
        return -1;
    }

    int ret = 0;
    for (int i = 1; tokens[i] != NULL; i++) {
        char *name = tokens[i];
        bool found = false;

        for (int j = 0; j < aliases.count; j++) {
            if (strcmp(aliases.table[j].name, name) == 0) {
                // Free the alias memory
                free(aliases.table[j].name);
                for (int k = 0; k < aliases.table[j].args_count; k++) {
                    free(aliases.table[j].args[k]);
                }
                free(aliases.table[j].args);

                // Shift the rest of the array left by 1
                for (int k = j; k < aliases.count - 1; k++) {
                    aliases.table[k] = aliases.table[k + 1];
                }

                aliases.count--;
                found = true;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "unalias: %s: not found\n", name);
            ret = 1;
        }
    }

    return ret;
}

// ---- .harshrc loading ----

/**
 * Load aliases from ~/.harshrc config file.
 * Each line is tokenized; lines starting with "alias" are processed
 * as alias commands. Lines starting with '#' and empty lines are skipped.
 * @return 0 on success, -1 on error (file not found is not an error)
 */
int load_harshrc(){
    char *home = getenv("HOME");
    if (home == NULL) return 0;

    char path[512];
    snprintf(path, sizeof(path), "%s/.harshrc", home);

    FILE *fp = fopen(path, "r");
    if (fp == NULL){
        // No config file is fine, not an error
        if (errno != ENOENT){
            perror(path);
            return -1;
        }
        return 0;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL){
        // Skip empty lines and comments
        if (line[0] == '\n' || line[0] == '#') continue;

        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        // Tokenize the line using the shell's tokenizer
        char *line_copy = strdup(line);
        if (line_copy == NULL){
            perror("strdup: harshrc line");
            continue;
        }
        char **tokens = tokenize(line_copy);
        free(line_copy);
        if (tokens == NULL) continue;
        // Process alias lines
        if (tokens[0] != NULL && strcmp(tokens[0], "alias") == 0){
            alias_command(tokens);
        }
        free_tokens(tokens);
    }

    fclose(fp);
    return 0;
}
