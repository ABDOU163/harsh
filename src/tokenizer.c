#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "globals.h"
#include <glob.h>

char* tilde_expander(char *token) {
    char* home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Warning: HOME environment variable not set\n");
        return token;  // Return original token if HOME not set
    }
    
    char* tilde_pos = strchr(token, '~');
    
    // If no tilde found, return original token
    if (!tilde_pos) {
        return token;
    }
    if (tilde_pos != token){
        home+=1;
    }
    
    size_t home_len = strlen(home);
    size_t new_len = strlen(token) - 1 + home_len + 1; // -1 for ~, +1 for \0
    
    char* new_str = malloc(new_len);
    if (!new_str) {
        fprintf(stderr, "Error: malloc failed\n");
        return token;  // Return original on failure
    }
    
    // Copy part before ~
    size_t prefix_len = tilde_pos - token;
    strncpy(new_str, token, prefix_len);
    new_str[prefix_len] = '\0';
    
    // Append HOME
    strcat(new_str, home);
    
    // Append part after ~
    strcat(new_str, tilde_pos + 1);
    
    return new_str;
}


void remove_trailing_spaces(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\n' || str[len - 1] == '\t')) {
        str[len - 1] = '\0';
        len--;
    }
}

void glob_expander_print_free(char *token){
    glob_t glob_result;
    int ret = glob(token, GLOB_TILDE | GLOB_MARK, NULL, &glob_result);
    if (ret == 0) {
        for (size_t i = 0; i < glob_result.gl_pathc; i++) {
            printf("%s\n", glob_result.gl_pathv[i]);
        }
        globfree(&glob_result);
    }
}



// Placeholder character for spaces inside quotes 
#define SPACE_PLACEHOLDER 'a'
// it is sufficient that the SPACE_PLACEHOLDER is not equal to any of the delimiters of strtok

// Pre-process: replace spaces inside quotes with placeholder
void preprocess_quotes(char *str, char **idx, int *size) {
    bool in_single_quote = false;
    bool in_double_quote = false;
    int i=0;
    for (char *p = str; *p != '\0'; p++) {
        if (*p == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
        } else if (*p == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
        } else if ((in_single_quote || in_double_quote) && (*p == ' ')) {
            *p = SPACE_PLACEHOLDER;
            idx[i++] = p;
        }
    }
    idx[i] = (char*)NULL;
    *size = i;
}

// Post-process: restore placeholders to spaces and strip surrounding quotes
char* postprocess_token(char *token, char **idx, int *count, int size, bool *quotes) {
    // Restore placeholder characters back to spaces
    for (char *p = token; *p != '\0'; p++) {
        if ((*count < size) && (*p == SPACE_PLACEHOLDER) && (idx[*count]==p)) {
            *p = ' ';
            *count +=1;
        }
    }
    size_t len = strlen(token);
    
    // Strip surrounding quotes if present
    if (len >= 2 && ((token[0] == '"' && token[len-1] == '"') ||
                     (token[0] == '\'' && token[len-1] == '\''))) {
        token[len-1] = '\0';  // Remove closing quote
        token++;              // Skip opening quote
        *quotes = true;
    }
    
    return token;
}

char** tokenize(char *command, bool *is_special) {
    remove_trailing_spaces(command);
    char **tokens = malloc((MAX_TOKENS + 1) * sizeof(char*));
    *is_special = false;
    int position = 0;
    char *idx[MAX_TOKENS];
    int count, size;
    bool quotes;
    
    if (!tokens) {
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Pre-process: protect spaces inside quotes
    preprocess_quotes(command, idx, &size);
    count=0;
    // Use strtok to tokenize
    char *token = strtok(command, " ");
    while (token != NULL) {
        if (position >= MAX_TOKENS) {
            fprintf(stderr, "Too many tokens\n");
            exit(EXIT_FAILURE);
        }
        
        // Post-process: restore spaces and strip quotes in token (not in command)
        quotes = false;
        char *processed_token = postprocess_token(token, idx, &count, size, &quotes);
        
        // Check if it's a special command
        if (!(*is_special)) {
            for (int j = 0; special_commands[j] != NULL; j++) {
                if (strcmp(processed_token, special_commands[j]) == 0) {
                    *is_special = true;
                    break;
                }
            }
        }
        
        // Apply tilde expansion
        if (quotes){
            // in this case, the token in the command is wrapped in quotes
            // as in: ... "..." ...
            tokens[position++] = strdup(processed_token);
        } else {
            glob_t glob_result;
            int ret=glob(processed_token, GLOB_TILDE | GLOB_MARK, NULL, &glob_result);
            if (ret == 0) {
                for (size_t i = 0; i < glob_result.gl_pathc; i++) {
                    tokens[position++] = strdup(glob_result.gl_pathv[i]);
                }
                globfree(&glob_result);
            } else if (ret == GLOB_NOMATCH){
                tokens[position++] = strdup(processed_token);
            } else {
                fprintf(stderr, "Glob error: %s\n", ret==1 ? "GLOB_NOSPACE" : "GLOB_ABORTED");
            }
        }
        
        token = strtok(NULL, " ");
    }
    
    tokens[position] = NULL;
    return tokens;
}