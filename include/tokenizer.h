#ifndef TOKENIZER_H
#define TOKENIZER_H
#include <stdbool.h>
char** tokenize_with_distinction(char *command, bool *is_special, int *which_special);
void remove_trailing_spaces(char *str);
#endif