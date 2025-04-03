CC = gcc
CFLAGS = -g -Wall -Wextra -pthread

# Source files
SRCS = main.c system.c resource.c event.c manager.c

# Object files (replace .c with .o for all source files)
OBJS = $(SRCS:.c=.o)

# Output executable
TARGET = simulation

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
