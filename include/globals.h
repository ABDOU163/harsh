#ifndef GLOBALS_H
#define GLOBALS_H

#define MAX_TOKENS_LIMIT 64
#define HISTORY_LENGTH 100

extern char *special_commands[11];
extern char *redirects[6];
extern void free_tokens(char **tokens);

typedef struct {
    // Pipe operators: |
    int pipe_pos[MAX_TOKENS_LIMIT];
    int pipe_count;

    // Logical operators: && (type=0), || (type=1)
    int andor_pos[MAX_TOKENS_LIMIT];
    int andor_types[MAX_TOKENS_LIMIT];
    int andor_count;

    // Redirect operators: >, >>, <, 2>, 2>>
    int redir_pos[MAX_TOKENS_LIMIT];
    int redir_count;

    // Sequencing operators: ; (type=0), & (type=1)
    int seqbg_pos[MAX_TOKENS_LIMIT];
    int seqbg_types[MAX_TOKENS_LIMIT];
    int seqbg_count;
} ops_t;

void scan_operators(char **tokens, ops_t *ops);

#endif