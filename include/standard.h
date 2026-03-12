#ifndef STANDARD_H
#define STANDARD_H

#include <stdbool.h>

int cd_handler(char **tokens);
int pushd_handler(char **tokens);
int popd_handler(char **tokens);
int init_dirstack();
bool is_builtin(const char *cmd);
int exec_builtin(char **tokens);
void exec_external(char **tokens);
int exec_standard(char **tokens);
#endif