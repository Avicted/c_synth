# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -lm -lportaudio

# Directories
SRC_DIR = src
BUILD_DIR = build

# Files
SRC = $(SRC_DIR)/main.c
OBJ = $(BUILD_DIR)/main.o
OUT = $(BUILD_DIR)/main

# Default target
all: $(OUT)

# Rule to link object files and generate the output binary
$(OUT): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJ) -o $(OUT) $(CFLAGS)

# Rule to compile source file into object file
$(OBJ): $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) -c $(SRC) -o $(OBJ) $(CFLAGS)

clean:
	rm -rf $(BUILD_DIR)

run: $(OUT)
	$(OUT)

.PHONY: all clean run
