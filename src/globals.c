#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"


char *special_commands[11] = {"|", "&", ";", "&&", "||", ">", ">>", "<", "2>", "2>>", NULL};
const int MAX_TOKENS = 64;
const int HISTORY_LENGTH = 100;
char *redirects[6] = {">", ">>", "<", "2>", "2>>", NULL};
void free_tokens(char **tokens){
    for (int i = 0; tokens[i] != NULL; i++){
        free(tokens[i]);
    }
    free(tokens);
}