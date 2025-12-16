// --------------------------------------------------------------------------------------------
// this version allocates only the pointer to tokens
// but it does not handle well ' and " in the command

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <stdbool.h>
// #include "globals.h"

// char* tilde_expander(char *token) {
//     char* home = getenv("HOME");
//     if (!home) {
//         fprintf(stderr, "Warning: HOME environment variable not set\n");
//         return token;  // Return original token if HOME not set
//     }
    
//     char* tilde_pos = strchr(token, '~');
    
//     // If no tilde found, return original token
//     if (!tilde_pos) {
//         return token;
//     }
//     if (tilde_pos != token){
//         home+=1;
//     }
    
//     size_t home_len = strlen(home);
//     size_t new_len = strlen(token) - 1 + home_len + 1; // -1 for ~, +1 for \0
    
//     char* new_str = malloc(new_len);
//     if (!new_str) {
//         fprintf(stderr, "Error: malloc failed\n");
//         return token;  // Return original on failure
//     }
    
//     // Copy part before ~
//     size_t prefix_len = tilde_pos - token;
//     strncpy(new_str, token, prefix_len);
//     new_str[prefix_len] = '\0';
    
//     // Append HOME
//     strcat(new_str, home);
    
//     // Append part after ~
//     strcat(new_str, tilde_pos + 1);
    
//     return new_str;
// }


// void remove_trailing_spaces(char *str) {
//     int len = strlen(str);
//     while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\n' || str[len - 1] == '\t')) {
//         str[len - 1] = '\0';
//         len--;
//     }
// }


// char** tokenize(char *command, bool *is_special) {
//     remove_trailing_spaces(command);
//     char **tokens = malloc((MAX_TOKENS+1) * sizeof(char*));
//     *is_special = false;
//     char *token;
//     int position = 0;
//     if (!tokens) {
//         fprintf(stderr, "Allocation error\n");
//         exit(EXIT_FAILURE);
//     }

//     token = strtok(command, " \t\r\n");
//     while (token != NULL) {
//         if (!(*is_special)){
//             for (int j=0 ; special_commands[j] != NULL ; j++){
//                 if (strcmp(token, special_commands[j]) == 0){
//                     *is_special = true;
//                     break;
//                 }
//             }
//         }
//         tokens[position] = tilde_expander(token);
//         position++;

//         if (position >= MAX_TOKENS) {
//             fprintf(stderr, "Too many tokens\n");
//             exit(EXIT_FAILURE);
//         }

//         token = strtok(NULL, " \t\r\n");
//     }
//     tokens[position] = NULL;
//     return tokens;
// }


// -------------------------------------------------------------------------------------------------
// This version allocates each token individually
// but it handles the quotes in the command


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


void remove_trailing_spaces(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\n' || str[len - 1] == '\t')) {
        str[len - 1] = '\0';
        len--;
    }
}


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