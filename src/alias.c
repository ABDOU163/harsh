#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <unistd.h>

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
    load_harshrc();

    return 0;
}

void free_alias_table(){
    for (int i = 0; i < aliases.count; i++){
        free(aliases.table[i].name);
        for (int j = 0; j < aliases.table[i].args_count; j++){
            free(aliases.table[i].args[j]);
        }
        free(aliases.table[i].args);
    }
    free(aliases.table);
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

// ---- Alias argument injection ----

/**
 * Inject alias replacement args into the token array.
 * Replaces tokens[0] and shifts existing user args right to make room
 * for the alias args. Uses memmove for the shift.
 *
 * Example: alias ll = {"ls", "-la", NULL}
 *   Before: ["ll", "foo/", NULL]
 *   After:  ["ls", "-la", "foo/", NULL]
 *
 * @param tokens     Token array (fixed size MAX_TOKENS_LIMIT + 1)
 * @param alias_args NULL-terminated replacement args
 * @param alias_argc Number of alias args (not counting NULL)
 * @return 0 on success, -1 if tokens would exceed MAX_TOKENS_LIMIT
 */
static int inject_args(char **tokens, char **alias_args, int alias_argc){
    // Count current tokens
    int total = 0;
    while (tokens[total] != NULL) total++;

    // Extra args to insert (alias_argc - 1, since alias_args[0] replaces tokens[0])
    int extra = alias_argc - 1;

    if (total + extra > MAX_TOKENS_LIMIT){
        fprintf(stderr, "%s: too many arguments after alias expansion\n", tokens[0]);
        return -1;
    }

    // Save original tokens[0] so we can restore on failure
    char *saved_token0 = tokens[0];

    // Shift tokens[1..total] right by 'extra' positions (including NULL terminator)
    if (extra > 0){
        memmove(&tokens[1 + extra], &tokens[1], sizeof(char*) * total);
    }

    // Insert alias args (strdup each)
    for (int i = 0; i < alias_argc; i++){
        tokens[i] = alias_args[i];
    }

    // Success: saved_token0 is still in the shifted array and will be freed by free_tokens()
    return 0;
}

int apply_aliases(char **tokens){
    if (tokens[0] == NULL) return 0;

    int applied[MAX_TOKENS_LIMIT];
    for (int i = 0; i < MAX_TOKENS_LIMIT; i++) {
        applied[i] = -1;
    }
    int applied_count = 0;

    bool expanded = true;
    while (expanded && tokens[0] != NULL) {
        expanded = false;
        
        for (int i = 0; i < aliases.count; i++){
            // Check if tokens[0] matches the alias name
            if (strcmp(tokens[0], aliases.table[i].name) == 0){
                
                // Check if this alias has already been applied in this chain
                bool cycle_detected = false;
                for (int j = 0; j < applied_count; j++) {
                    if (applied[j] == i) {
                        cycle_detected = true;
                        break;
                    }
                }
                
                // If it's a cycle, we stop expanding and just use the current tokens
                if (cycle_detected) {
                    break;
                }

                // Inject arguments starting at index 0
                if (inject_args(tokens, aliases.table[i].args, aliases.table[i].args_count) != 0) {
                    return -1; // MAX_TOKENS_LIMIT exceeded
                }
                
                // Mark this alias as applied
                applied[applied_count++] = i;
                
                // Set expanded to true so the while loop runs again on the NEW tokens[0]
                expanded = true;
                break; // Break the for loop, restart the while loop
            }
        }
    }
    return 0;
}

// ---- alias command (interactive + .harshrc) ----

int valid_alias_command(char *token, char *name, char *cmd){
    char *p=strchr(token, '=');
    if (!p || (p-token == 0) || (p[1] != '"') || (p[strlen(p)-1] != '"')){
        fprintf(stderr, "alias: usage: alias name=\"command [args...]\"\n");
        return -1;
    }
    *p='\0';
    name = token;
    cmd = p+2;
    cmd[strlen(cmd)-1]= '\0';
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

    // usage: fprintf(stderr, "alias: usage: alias name=\"command [args...]\"\n");
    for (int j=1; tokens[j] != NULL; j++){
        char *name;
        char *cmd;
        int count;
        if (valid_alias_command(tokens[j], name, cmd) != 0){
            printf("%s: not found", tokens[j]);
            continue;
        }
        char **cmd_args = tokenize(cmd);
        if (!cmd_args){
            fprintf(stderr, "Failed to alias command: %s\n", cmd);
            continue;
        }
        for (count=0; cmd_args[count] != NULL; count++);
        if (add_alias(name, cmd_args, count) != 0){
            fprintf(stderr, "alias: failed to add alias '%s'\n", name);
            return -1;
        }
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
        if (line_copy == NULL) continue;

        char **tokens = tokenize(line_copy);
        free(line_copy);
        if (tokens == NULL) continue;
        for (int i=0; tokens[i]; i++){
            printf("%s\n", tokens[i]);
        }

        // Process alias lines
        if (tokens[0] != NULL && strcmp(tokens[0], "alias") == 0){
            alias_command(tokens);
        }
        free_tokens(tokens);
    }

    fclose(fp);
    return 0;
}
