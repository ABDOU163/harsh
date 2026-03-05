#include "errors.h"
#include <stddef.h>

static const char *error_strings[] = {
    [SH_SUCCESS]              = "Success",

    // Memory errors
    [SH_ERR_MALLOC]           = "Memory allocation failed",

    // Process errors
    [SH_ERR_FORK]             = "Fork failed",
    [SH_ERR_EXEC]             = "Exec failed",
    [SH_ERR_WAITPID]          = "Waitpid failed",

    // Pipe errors
    [SH_ERR_PIPE]             = "Pipe creation failed",

    // File descriptor errors
    [SH_ERR_DUP]              = "dup failed",
    [SH_ERR_DUP2]             = "dup2 failed",
    [SH_ERR_OPEN]             = "File open failed",
    [SH_ERR_CLOSE]            = "File close failed",
    [SH_ERR_SAVE_FDS]         = "Failed to save file descriptors",

    // Redirect errors
    [SH_ERR_INVALID_REDIRECT] = "Invalid redirect operator",
    [SH_ERR_REDIRECT_SETUP]   = "Redirection setup failed",

    // Command errors
    [SH_ERR_CMD_NOT_FOUND]    = "Command not found",
    [SH_ERR_INVALID_OPERATOR] = "Invalid special operator",
    [SH_ERR_TOO_MANY_TOKENS]  = "Too many tokens",

    // Built-in command errors
    [SH_ERR_CD_MISSING_ARG]   = "cd: expected argument",
    [SH_ERR_CD_TOO_MANY_ARGS] = "cd: too many arguments",
    [SH_ERR_CD_FAILED]        = "cd: directory change failed",

    // Environment errors
    [SH_ERR_HOME_NOT_SET]     = "HOME environment variable not set",
    [SH_ERR_GETENV]           = "Failed to get environment variable",

    // Glob errors
    [SH_ERR_GLOB_NOSPACE]     = "Glob error: GLOB_NOSPACE",
    [SH_ERR_GLOB_ABORTED]     = "Glob error: GLOB_ABORTED",

    // Signal errors
    [SH_ERR_SIGEMPTYSET]      = "sigemptyset failed",
    [SH_ERR_SIGACTION]        = "sigaction failed",

    // Readline/IO errors
    [SH_ERR_EOF]              = "End of input (EOF)",
};

const char *sh_error_str(sh_error_t err) {
    if (err < 0 || err >= SH_ERR_COUNT) {
        return "Unknown error";
    }
    return err==0 ? NULL : error_strings[err];
}
