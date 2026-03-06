#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"


char *special_commands[11] = {"|", "&", ";", "&&", "||", ">", ">>", "<", "2>", "2>>", NULL};
char *redirects[6] = {">", ">>", "<", "2>", "2>>", NULL};
/**
 * Free all tokens in a NULL-terminated token array, then free the array itself.
 * @param tokens NULL-terminated array of heap-allocated strings
 */
void free_tokens(char **tokens){
    for (int i = 0; tokens[i] != NULL; i++){
        free(tokens[i]);
    }
    free(tokens);
}

/**
 * Scan all tokens in a single pass and classify operators into categories.
 * Populates ops with positions and types of pipes, &&/||, redirects, and ;/&.
 * @param tokens NULL-terminated token array
 * @param ops    Output struct to populate with operator positions and counts
 */
void scan_operators(char **tokens, ops_t *ops){
    ops->pipe_count = 0;
    ops->andor_count = 0;
    ops->redir_count = 0;
    ops->seqbg_count = 0;

    for (int i = 0; tokens[i] != NULL; i++){
        char *t = tokens[i];
        if (strcmp(t, "|") == 0){
            ops->pipe_pos[ops->pipe_count++] = i;
        } else if (strcmp(t, "&&") == 0){
            ops->andor_pos[ops->andor_count] = i;
            ops->andor_types[ops->andor_count] = 0;
            ops->andor_count++;
        } else if (strcmp(t, "||") == 0){
            ops->andor_pos[ops->andor_count] = i;
            ops->andor_types[ops->andor_count] = 1;
            ops->andor_count++;
        } else if (strcmp(t, ";") == 0){
            ops->seqbg_pos[ops->seqbg_count] = i;
            ops->seqbg_types[ops->seqbg_count] = 0;
            ops->seqbg_count++;
        } else if (strcmp(t, "&") == 0){
            ops->seqbg_pos[ops->seqbg_count] = i;
            ops->seqbg_types[ops->seqbg_count] = 1;
            ops->seqbg_count++;
        } else {
            // Check redirect operators
            for (int j = 0; redirects[j] != NULL; j++){
                if (strcmp(t, redirects[j]) == 0){
                    ops->redir_pos[ops->redir_count++] = i;
                    break;
                }
            }
        }
    }
}