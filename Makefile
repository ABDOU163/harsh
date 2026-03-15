CC = gcc
CFLAGS = -g -I./include
LDLIBS = -lreadline
# -Wall all warnings
# 1. Edit this line to change the output name
TARGET = harsh

# 2. Edit this line to add/remove .c files
# All listed files are compiled and linked in one step.
SRCS = main.c src/*.c

# Default rule: Build the executable
all: $(TARGET)

# Linking/Compilation Rule: Creates the executable directly from the source files
$(TARGET): $(SRCS)
	@$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDLIBS)

# Run rule
run: $(TARGET)
	@./$(TARGET)

# Clean rule: Only deletes the final binary (no .o files to delete)
clean:
	@rm -f $(TARGET)

# Test rule: use valgrind for testing for heap exploits
vg:
	@valgrind --trace-children=no \
	--leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--suppressions=readline.supp \
	./$(TARGET)


# compile and run test file with valgrind
SRC = test.c
OUT = test.bin
# --show-leak-kinds=definite,indirect,possible 
vg-test:
	@$(CC) $(SRC) -o $(OUT) ; 
	@valgrind --trace-children=no \
	--leak-check=full \
	--show-leak-kinds=definite,indirect,possible \
	--track-origins=yes \
	--suppressions=readline.supp \
	./$(OUT)

run-test:
	@$(CC) $(SRC) -o $(OUT) ; 
	@./$(OUT)

test:
	@chmod +x test.sh && bash test.sh