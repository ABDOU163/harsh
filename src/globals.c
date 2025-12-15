#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"


char *special_commands[11] = {"|", "&", ";", "&&", "||", ">", ">>", "<", "2>", "2>>", NULL};
const int MAX_TOKENS = 64;