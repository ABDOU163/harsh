
# 1. Variables
CC = gcc
CFLAGS = -Wall -std=c11 -g

# TARGET: The name of the final executable file. 
# We explicitly add the .exe extension for Windows compatibility.
TARGET = standard_commands.exe 

# SRCS: List of all source files (.c)
SRCS = standard_commands.c globals.c

# OBJS: Object files calculation remains the same
OBJS = $(patsubst %.c,%.o,$(SRCS))

# ----------------------------------------------------------------------

# 2. Main Target: The default target
all: $(TARGET)

# Rule to create the final executable (Linking)
# The output file name ($@) now includes the .exe extension.
$(TARGET): $(OBJS)
	@echo "--- Linking executable: $@ ---"
	$(CC) $(CFLAGS) $^ -o $@

# 3. Generic Rule for Compiling Source Files (No change needed here)
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# ----------------------------------------------------------------------

# 4. Utility Targets

# clean: Removes all generated files, including the .exe
.PHONY: clean
clean:
	@echo "--- Cleaning up project files ---"
	$(RM) $(OBJS) $(TARGET)

# run: Builds the project and then executes the TARGET
# On Windows, you typically execute the file name directly (./target.exe)
# However, many shells (like PowerShell or Git Bash/MinGW) accept both ./target and ./target.exe
.PHONY: run
run: $(TARGET)
	@echo "--- Running $(TARGET) ---"
	./$(TARGET)