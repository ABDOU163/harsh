#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "globals.h"
#include <glob.h>


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

char** tokenize(char *command) {
    remove_trailing_spaces(command);
    char **tokens = malloc((MAX_TOKENS + 1) * sizeof(char*));
    int position = 0;
    char *idx[MAX_TOKENS];
    int count, size;
    bool quotes;
    
    if (!tokens) {
        fprintf(stderr, "Allocation error\n");
        return NULL;
    }

    // Pre-process: protect spaces inside quotes
    preprocess_quotes(command, idx, &size);
    count=0;
    // Use strtok to tokenize
    char *token = strtok(command, " ");
    while (token != NULL) {
        if (position >= MAX_TOKENS) {
            fprintf(stderr, "Too many tokens\n");
            free_tokens(tokens);
            return NULL;
        }
        
        // Post-process: restore spaces and strip quotes in token (not in command)
        quotes = false;
        char *processed_token = postprocess_token(token, idx, &count, size, &quotes);
        
        // Apply tilde expansion
        if (quotes){
            // in this case, the token in the command is wrapped in quotes
            // as in: ... "..." ...
            char *dup = strdup(processed_token);
            if (!dup) { fprintf(stderr, "Allocation error\n"); free_tokens(tokens); return NULL; }
            tokens[position++] = dup;
        } else {
            glob_t glob_result;
            int ret=glob(processed_token, GLOB_TILDE | GLOB_MARK, NULL, &glob_result);
            if (ret == 0) {
                for (size_t i = 0; i < glob_result.gl_pathc; i++) {
                    if (position >= MAX_TOKENS) {
                        fprintf(stderr, "Too many tokens (glob expansion)\n");
                        globfree(&glob_result);
                        free_tokens(tokens);
                        return NULL;
                    }
                    char *dup = strdup(glob_result.gl_pathv[i]);
                    if (!dup) { fprintf(stderr, "Allocation error\n"); globfree(&glob_result); free_tokens(tokens); return NULL; }
                    tokens[position++] = dup;
                }
                globfree(&glob_result);
            } else if (ret == GLOB_NOMATCH){
                char *dup = strdup(processed_token);
                if (!dup) { fprintf(stderr, "Allocation error\n"); free_tokens(tokens); return NULL; }
                tokens[position++] = dup;
            } else {
                fprintf(stderr, "Glob error: %s\n", ret==1 ? "GLOB_NOSPACE" : "GLOB_ABORTED");
            }
        }
        
        token = strtok(NULL, " ");
    }
    
    tokens[position] = NULL;
    return tokens;
}