# ----------------------------------------
# Makefile to compile and link C source files

# Author: madbookhub@github

# Created: Aug,18,2024

# Recent: Oct,6,2024
# ----------------------------------------
# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -w # -Wall

# Source files
SOURCES = common.c term.c proxy.c executant.c main.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Executable name
EXECUTABLE = ladbroker

# Default target
all: $(EXECUTABLE)

# Link object files to create the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean
