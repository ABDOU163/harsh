#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "globals.h"

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

// All of the following versions used tilde_expander, but the version in tokenizer.c uses glob directly for tilde and *,? expanding

// --------------------------------------------------------------------------------------------
// this version allocates only the pointer to tokens
// but it does not handle well ' and " in the command

char** tokenize(char *command, bool *is_special) {
    remove_trailing_spaces(command);
    char **tokens = malloc((MAX_TOKENS+1) * sizeof(char*));
    *is_special = false;
    char *token;
    int position = 0;
    if (!tokens) {
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(command, " \t\r\n");
    while (token != NULL) {
        if (!(*is_special)){
            for (int j=0 ; special_commands[j] != NULL ; j++){
                if (strcmp(token, special_commands[j]) == 0){
                    *is_special = true;
                    break;
                }
            }
        }
        tokens[position] = tilde_expander(token);
        position++;

        if (position >= MAX_TOKENS) {
            fprintf(stderr, "Too many tokens\n");
            exit(EXIT_FAILURE);
        }

        token = strtok(NULL, " \t\r\n");
    }
    tokens[position] = NULL;
    return tokens;
}




// -------------------------------------------------------------------------------------------------
// This version allocates each token individually
// but it handles the quotes in the command

char** tokenize(char *command, bool *is_special) {
    remove_trailing_spaces(command);
    char **tokens = malloc((MAX_TOKENS+1) * sizeof(char*));
    *is_special = false;
    int position = 0;
    
    if (!tokens) {
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
    }

    char *ptr = command;
    
    // Skip leading whitespace
    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') ptr++;
    
    while (*ptr != '\0') {
        if (position >= MAX_TOKENS) {
            fprintf(stderr, "Too many tokens\n");
            exit(EXIT_FAILURE);
        }
        
        char *token_start = ptr;
        int token_len = 0;
        char quote_char = '\0';
        
        // Check if token starts with a quote
        if (*ptr == '"' || *ptr == '\'') {
            quote_char = *ptr;
            ptr++; // Skip opening quote
            token_start = ptr;
            
            // Find closing quote
            while (*ptr != '\0' && *ptr != quote_char) {
                token_len++;
                ptr++;
            }
            
            if (*ptr == quote_char) {
                ptr++; // Skip closing quote
            }
        } else {
            // Regular token - read until space
            while (*ptr != '\0' && *ptr != ' ' && *ptr != '\t' && *ptr != '\n') {
                token_len++;
                ptr++;
            }
        }
        
        // Copy token
        if (token_len > 0) {
            char *token = malloc(token_len + 1);
            if (!token) {
                fprintf(stderr, "Allocation error\n");
                exit(EXIT_FAILURE);
            }
            strncpy(token, token_start, token_len);
            token[token_len] = '\0';
            
            // Check if it's a special command
            if (!(*is_special)) {
                for (int j = 0; special_commands[j] != NULL; j++) {
                    if (strcmp(token, special_commands[j]) == 0) {
                        *is_special = true;
                        break;
                    }
                }
            }
            
            // Apply tilde expansion
            tokens[position] = tilde_expander(token);
            position++;
        }
        
        // Skip whitespace
        while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') ptr++;
    }
    
    tokens[position] = NULL;
    return tokens;
}


// -------------------------------------------------------------------------------------------------
// This version combines the best of both worlds:
// - Handles quotes properly (content inside quotes is a single token)
// - Uses strtok for tokenization after pre-processing quotes
// - Points tokens directly to the command string (minimal malloc)
// - Only allocates when tilde expansion is needed

// Placeholder character for spaces inside quotes (must not appear in normal input)
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
        
        // Post-process: restore spaces and strip quotes
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
            tokens[position] = processed_token;
        } else {
            tokens[position] = tilde_expander(processed_token);
        }
        position++;
        
        token = strtok(NULL, " ");
    }
    
    tokens[position] = NULL;
    return tokens;
}



// iterative method for match function for wildcard expansion (* and ?)
bool matches_pattern_iterative(char *pattern, char *candidate) {
    char *p = pattern;
    char *s = candidate;
    char *last_star = NULL;
    char *s_star_match_pos = NULL;

    while (*s != '\0') {
        if (*p == *s || *p == '?') {
            p++;
            s++;
        } else if (*p == '*') {
            last_star = p;
            s_star_match_pos = s;
            p++;
        } else if (last_star != NULL) {
            p = last_star + 1;
            s_star_match_pos++;
            s = s_star_match_pos;
        } else {
            return false;
        }
    }

    while (*p == '*') {
        p++;
    }

    return *p == '\0';
}
