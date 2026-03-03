#ifndef GLOBALS_H
#define GLOBALS_H

extern char *special_commands[11];
extern char*redirects[6];
extern const int MAX_TOKENS;
extern const int HISTORY_LENGTH;
extern void free_tokens(char **tokens);

#endif