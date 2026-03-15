# Harsh Shell 

A robust, feature-rich, and heavily optimized custom Unix shell written in C from scratch. `harsh` implements advanced shell capabilities natively while prioritizing extreme memory safety and zero memory leaks.

## Key Features

Harsh goes beyond a simple `fork()`/`exec()` loop and fully supports complex standard Unix shell behaviors:

### Core Capabilities
- **Pipelines (`|`)**: Support for arbitrarily long chains of piped commands (e.g., `ls -l | grep ".c" | wc -l`).
- **Redirection**: Full support for input/output & error redirection (`>`, `>>`, `<`, `2>`, `2>>`).
- **Logical Operators (`&&`, `||`)**: Short-circuiting logical execution paths.
- **Sequencing (`;`, `&`)**: Run trailing sequential commands (`;`) and background, non-blocking asynchronous execution (`&`).

### Advanced Expansion & Tokenization
- **Globbing**: Native support for wildcard expansion (e.g., `ls *.c`, using `glob` or manual implementation).
- **Tilde Expansion**: Automatically expands `~` to the user's `$HOME` directory.
- **Quote Handling**: Proper parsing and preservation of spaces within single (`' '`) and double (`" "`) quotes.
- **History Expansion**: Bang commands like `!!` (run last command), `!n`, `!-n`, and `!string`.

### Shell Builtins & Environment
- **Directory Stack**: Full `pushd` and `popd` implementation for fast directory stack manipulation, alongside the standard `cd`.
- **Alias Management**: Define aliases on the fly (`alias ll ls -la`), execute them and print them. 
- **Initialization Script**: Automatically locates, loads, and parses `~/.harshrc` on startup to inject defaults.
- **Line Editing**: Arrow-key navigation, reverse-i-search, tab-completion, and history powered by GNU `readline`.

## Bulletproof Memory Safety
Harsh was built with strict memory hygiene in mind. The shell successfully evaluates heavily nested command trees while registering **0 bytes leaked** and **0 bytes "still reachable"** natively in Valgrind, even across deeply failing child processes and invalid `execvp()` execution paths.

## Why `readline.supp`?
The GNU `readline` library intentionally leaves behind internal terminal buffers upon process exit. We use `--suppressions=readline.supp` during testing simply to filter out these unavoidable third-party allocations, proving our core shell codebase is completely spotless!

## Project Structure

```text
harsh/
├── src/                # Core C implementation (special commands, pipelines, builtins)
├── include/            # Headers and globals
├── other/              # Exploratory & Manual Implementations
├── main.c              # REPL loop, history expansion, and shell execution
├── test.sh             # Valgrind memory leak automated test suite
├── readline.supp       # Valgrind suppressions for GNU Readline
└── Makefile            # Build system
```
## Note: 
The `other/` directory contains alternative, manually written C implementations of complex standard functions (like custom `glob` matching) and different tokenization parser variants. Each has its own trade-offs, advantages, and limitations, preserved here for reference and lower-level study.

## Building and Running

### Prerequisites
- GCC / Clang
- GNU Readline (`libreadline-dev` / `readline-devel`)
- Valgrind (for testing)

### Build Instructions
```bash
# Compile the shell
make

# Run the shell
./harsh

# Run the automated Valgrind memory leak test suite
make test
```
