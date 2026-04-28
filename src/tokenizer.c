#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "globals.h"
#include <glob.h>


/**
 * Remove trailing whitespace (spaces, newlines, tabs) from a string in-place.
 * @param str String to trim (modified in-place)
 */
void remove_trailing_spaces(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\n' || str[len - 1] == '\t')) {
        str[len - 1] = '\0';
        len--;
    }
}


char* tilde_expander(char *token) {
    char* home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Warning: HOME environment variable not set\n");
        return token;  // Return original token if HOME not set
    }
    
    char* tilde_pos = strchr(token, '~');
    
    // If no tilde found, return original token
    if (!tilde_pos) {
        // printf("Tilde was not found\n");
        return strdup(token);
    }
    if (tilde_pos != token){
        home+=1;
    }
    
    size_t home_len = strlen(home);
    size_t new_len = strlen(token) - 1 + home_len + 1; // -1 for ~, +1 for \0
    
    char* new_str = malloc(new_len);
    if (!new_str) {
        return NULL;
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



// Placeholder character for spaces inside quotes 
#define SPACE_PLACEHOLDER 'a'
// it is sufficient that the SPACE_PLACEHOLDER is not equal to any of the delimiters of strtok

/**
 * Pre-process a command string: replace spaces inside quotes with a placeholder
 * character so that strtok does not split quoted strings.
 * @param str  Command string (modified in-place)
 * @param idx  Output array of pointers to replaced characters (for restoration)
 * @param size Output: number of replacements made
 */
void preprocess_quotes(char *str, char **idx, int *size) {
    bool in_single_quote = false;// ''
    bool in_double_quote = false;// ""
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

/**
 * Post-process a token: restore placeholder characters back to spaces and
 * strip surrounding quotes if present.
 * @param token  Token string (modified in-place)
 * @param idx    Array of pointers to placeholder positions (from preprocess_quotes)
 * @param count  In/out: current index into idx array
 * @param size   Total number of placeholders
 * @param quotes Output: set to true if surrounding quotes were stripped
 * @return Pointer to the processed token (may be offset from input if quotes stripped)
 */
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



/**
 * Tokenize a command string into a NULL-terminated array of strings.
 * Handles quoted strings, tilde expansion, and produces heap-allocated
 * copies of each token. The caller must free the result with free_tokens().
 * @param command Raw command string (modified in-place by strtok)
 * @return NULL-terminated array of heap-allocated token strings, or NULL on error
 */
char** tokenize(char *command) {
    // printf("%s\n", command);
    remove_trailing_spaces(command);
    char **tokens = malloc((MAX_TOKENS_LIMIT + 1) * sizeof(char*));
    int position = 0;
    char *idx[MAX_TOKENS_LIMIT];
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
        if (position >= MAX_TOKENS_LIMIT) {
            fprintf(stderr, "Too many tokens\n");
            free_tokens(tokens);
            return NULL;
        }
        tokens[position]=NULL;
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
            // apply only tilde expansion
            // leave globbing for later ( because of aliasing, so we need to do glob just before executing)
            char *expanded = tilde_expander(processed_token);
            if (!expanded) {fprintf(stderr, "Allocation error\n"); free_tokens(tokens); return NULL;}
            tokens[position++] = expanded;
        }
        
        token = strtok(NULL, " ");
    }
    
    tokens[position] = NULL;
    return tokens;
}

/**
 * Expand wildcard patterns in a NULL-terminated token array using glob().
 * Each match becomes a separate token; non-matching patterns are kept as-is.
 *
 * Returns a new heap-allocated NULL-terminated array.
 * Caller must free with free_tokens().
 *
 * @param tokens Input token array
 * @return Expanded token array, or NULL on error
 */
char  **glob_expansion(char **tokens){
    char **res=malloc(sizeof(char*) * (MAX_TOKENS_LIMIT+1));
    int position = 0;
    for (int j=0; tokens[j] != NULL; j++){
        glob_t glob_result;
        int ret=glob(tokens[j], GLOB_TILDE | GLOB_MARK, NULL, &glob_result);
        if (ret == 0) {
            for (size_t i = 0; i < glob_result.gl_pathc; i++) {
                if (position >= MAX_TOKENS_LIMIT) {
                    fprintf(stderr, "Too many tokens (glob expansion)\n");
                    globfree(&glob_result);
                    free_tokens(res);
                    return NULL;
                }
                char *dup = strdup(glob_result.gl_pathv[i]);
                res[position++] = dup;
                if (!dup) { fprintf(stderr, "Allocation error\n"); globfree(&glob_result); free_tokens(tokens); return NULL; }
            }
            globfree(&glob_result);
        } else if (ret == GLOB_NOMATCH){
            if (position >= MAX_TOKENS_LIMIT) {
                fprintf(stderr, "Too many tokens (glob expansion)\n");
                globfree(&glob_result);
                free_tokens(res);
                return NULL;
            }
            char *dup = strdup(tokens[j]);
            res[position++] = dup;
            globfree(&glob_result);
            if (!dup) { 
                fprintf(stderr, "Allocation error\n");
                free_tokens(res); 
                return NULL; 
            }
        } else {
            fprintf(stderr, "Glob error: %s\n", ret==1 ? "GLOB_NOSPACE" : "GLOB_ABORTED");
            globfree(&glob_result); 
            res[position] = NULL;
            free_tokens(res);
            return NULL;
        }
    }
    res[position] = NULL;
    return res;
}