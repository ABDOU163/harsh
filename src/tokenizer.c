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


char** tokenize_with_distinction(char *command, bool *is_special, int *which_special) {
    remove_trailing_spaces(command);
    char **tokens = malloc(64 * sizeof(char*));
    char *token;
    int position = 0;

    if (!tokens) {
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(command, " \t\r\n");
    while (token != NULL) {
        for (int j=0 ; special_commands[j] != NULL ; j++){
            if (strcmp(token, special_commands[j]) == 0){
                *is_special = true;
                *which_special = j;
                break;
            }
        }
        tokens[position] = token;
        position++;

        if (position >= 64) {
            fprintf(stderr, "Too many tokens\n");
            exit(EXIT_FAILURE);
        }

        token = strtok(NULL, " \t\r\n");
    }
    tokens[position] = NULL;
    return tokens;
}

