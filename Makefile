CC = gcc
CFLAGS = -Wall -g

# 1. Edit this line to change the output name
TARGET = shell

# 2. Edit this line to add/remove .c files
# All listed files are compiled and linked in one step.
SRCS = *.c

# Default rule: Build the executable
all: $(TARGET)

# Linking/Compilation Rule: Creates the executable directly from the source files
$(TARGET): $(SRCS)
	@$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

# Run rule
run: $(TARGET)
	@./$(TARGET)

# Clean rule: Only deletes the final binary (no .o files to delete)
clean:
	@rm -f $(TARGET)
