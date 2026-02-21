----------------------------
Custom Shell Feature Roadmap
----------------------------

1. Core Command Execution
- fork a child for each command
- use exec() to run standard commands
- create a helper function to execute standard commands
  (this simplifies handling redirection, pipes, and history)

2. Command Parsing
- Manual tokenization of input
- Support arguments, quotes, escapes
- Handle special characters: * and ? for file matching

3. Input/Output Redirection
- Support '>' for stdout redirection
- Support '<' for stdin redirection
- Support '>>' for appending

4. Pipes
- Support standard pipes: cmd1 | cmd2 | cmd3
- Enable interactive pipeline mode:
  $ pipeline
    cat file
    grep foo
    sort
    run

5. Aliases & Directory Teleportation
- Implement alias feature for commands
- Implement quick directory jumps (like bookmarks)

6. Session-Based Checkpoints
- Save current working directory (cwd) as a checkpoint
- Restore to a checkpoint later
- Checkpoints exist only in current shell session

7. Time-Travel Feature
- Repeat last N commands
- Store for each command:
    - cwd
    - command string
    - exit code
- Allow easy replay

8. Other Enhancements
- List all checkpoints
- Combine checkpoints with time-travel for efficient debugging
- History navigation (up/down arrows if you want) and tab completion
