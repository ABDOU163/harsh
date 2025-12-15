#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "globals.h"


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
        tokens[position] = token;
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

