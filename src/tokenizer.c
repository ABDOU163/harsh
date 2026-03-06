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

/**
 * Pre-process a command string: replace spaces inside quotes with a placeholder
 * character so that strtok does not split quoted strings.
 * @param str  Command string (modified in-place)
 * @param idx  Output array of pointers to replaced characters (for restoration)
 * @param size Output: number of replacements made
 */
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
 * Handles quoted strings, glob/tilde expansion, and produces heap-allocated
 * copies of each token. The caller must free the result with free_tokens().
 * @param command Raw command string (modified in-place by strtok)
 * @return NULL-terminated array of heap-allocated token strings, or NULL on error
 */
char** tokenize(char *command) {
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
                    if (position >= MAX_TOKENS_LIMIT) {
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