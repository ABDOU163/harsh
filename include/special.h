#ifndef SPECIAL_H
#define SPECIAL_H

#include "globals.h"

int special_commands_run(char **tokens, ops_t *ops, int start, int end);
void command_run(char **tokens, ops_t *ops, int start, int end);
#endif