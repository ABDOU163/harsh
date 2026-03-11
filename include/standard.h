#ifndef STANDARD_H
#define STANDARD_H

int cd_handler(char **tokens);
void exec_standard(char **tokens);

// ---- Alias types and globals ----

#define INIT_ALIAS_CAPACITY 2

typedef struct alias {
    char *name;     // alias name, e.g. "ll"
    char **args;    // full replacement: {"ls", "-la", NULL}
    int args_count; // number of args (not counting NULL)
} alias_t;

typedef struct alias_manager {
    alias_t *table;
    int count;
    int capacity;
} alias_manager_t;

extern alias_manager_t aliases;

int init_alias_table();
int add_alias(const char *name, char **args, int count);
int apply_aliases(char **tokens);
int alias_command(char **tokens);
int load_harshrc();

#endif