#ifndef ERRORS_H
#define ERRORS_H

typedef enum {
    SH_SUCCESS = 0,

    // Memory errors
    SH_ERR_MALLOC,

    // Process errors
    SH_ERR_FORK,
    SH_ERR_EXEC,
    SH_ERR_WAITPID,

    // Pipe errors
    SH_ERR_PIPE,

    // File descriptor errors
    SH_ERR_DUP,
    SH_ERR_DUP2,
    SH_ERR_OPEN,
    SH_ERR_CLOSE,
    SH_ERR_SAVE_FDS,

    // Redirect errors
    SH_ERR_INVALID_REDIRECT,
    SH_ERR_REDIRECT_SETUP,

    // Command errors
    SH_ERR_CMD_NOT_FOUND,
    SH_ERR_INVALID_OPERATOR,
    SH_ERR_TOO_MANY_TOKENS,

    // Built-in command errors
    SH_ERR_CD_MISSING_ARG,
    SH_ERR_CD_TOO_MANY_ARGS,
    SH_ERR_CD_FAILED,

    // Environment errors
    SH_ERR_HOME_NOT_SET,
    SH_ERR_GETENV,

    // Glob errors
    SH_ERR_GLOB_NOSPACE,
    SH_ERR_GLOB_ABORTED,

    // Signal errors
    SH_ERR_SIGEMPTYSET,
    SH_ERR_SIGACTION,

    // Readline/IO errors
    SH_ERR_EOF,

    SH_ERR_COUNT  // always last, gives total error count
} sh_error_t;

const char *sh_error_str(sh_error_t err);

#endif
