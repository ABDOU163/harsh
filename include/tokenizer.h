#ifndef TOKENIZER_H
#define TOKENIZER_H
#include <stdbool.h>
char** tokenize(char *command);
void remove_trailing_spaces(char *str);
char* tilde_expander(char *token);
#endif