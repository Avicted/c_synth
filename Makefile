# Compiler and flags
CC = gcc
CFLAGS = -std=c23 -O0 -ggdb -Wall -Werror -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function
LINKER_FLAGS = -lm -lportaudio

# Directories
SRC_DIR = src
BUILD_DIR = build

# Files
SRC = $(SRC_DIR)/main.c
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c)) 
OUT = $(BUILD_DIR)/main

# Default target
all: $(OUT)

# Rule to link object files and generate the output binary
$(OUT): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) -o $(OUT) $(CFLAGS) $(LINKER_FLAGS)

# Rule to compile source file into object file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c $< -o $@ $(CFLAGS) $(LINKER_FLAGS)

clean:
	rm -rf $(BUILD_DIR)

run: $(OUT)
	$(OUT)

.PHONY: all clean run
