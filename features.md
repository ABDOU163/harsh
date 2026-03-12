----------------------------
Custom Shell Feature Roadmap
----------------------------

1. Core Command Execution ✔️
- fork a child for each command ✔️
- use exec() to run standard commands ✔️
- create a helper function to execute standard commands ✔️
  (this simplifies handling redirection, pipes, and history)

2. Command Parsing ✔️
- Manual tokenization of input ✔️
- Support arguments, quotes, escapes ✔️
- Handle special characters: * and ? for file matching ✔️

3. Input/Output Redirection ✔️
- Support stdout, stdin, stderr redirection ✔️
- Support appending ✔️

4. Pipes and others ✔️
- Support standard pipes and other operators: ; && || & ✔️

5- Unify code structure: ✔️
- consistent error handling ✔️
- consistent function signatures (e.g., all functions that can fail should return int) ✔️

6. Aliases & Directory Teleportation ✔️
- Implement alias feature for commands + --color=auto for ls, grep etc... ✔️
- Implement quick directory jumps (like bookmarks) ✔️

7. Session-Based Checkpoints
- pushd and popd ✔️

8. Other Enhancements
- History navigation (up/down arrows if you want) and tab completion + !! and !n ✔️

9. fwatch <file_or_dir> with inotify

10. jobs, fg, bg

11. code docs like linux kernel, example: ✔️
/**
 * Gets a string value.
 * @param input Input value
 * @param output Pointer to store the result string (caller must free)
 * @return 0 on success, negative error code on failure
 */