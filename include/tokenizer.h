#ifndef TOKENIZER_H
#define TOKENIZER_H
#include <stdbool.h>
char** tokenize(char *command, bool *is_special);
void remove_trailing_spaces(char *str);
#endif