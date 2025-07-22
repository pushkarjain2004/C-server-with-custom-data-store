# Define the C compiler to use
CC = cc # Or gcc, depending on your system setup (cc usually maps to clang on macOS)

# Define compiler flags (for compiling .c to .o)
# -O2: Optimization level 2
# -Wall: Enable all common warnings
# -std=c2x: Use C2x standard (or c11 if you prefer)
CFLAGS = -O2 -Wall -std=c2x

# Define any linker flags (e.g., -lm for math library, if needed)
# Not strictly needed for this project but good to have a placeholder
LDFLAGS =

# Define the name of the final executable
TARGET = cache22_server

# List all object files that make up your final executable
# Each .c file will compile into a .o file
OBJS = cache22.o tree.o

# ----------------- Rules -----------------

# Default target: builds the 'all' target
.PHONY: all clean

all: $(TARGET)

# Rule to link the object files into the final executable
# $(TARGET) depends on all object files listed in OBJS
# $@: expands to the target name (cache22_server)
# $^: expands to all prerequisites (cache22.o tree.o)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Rule to compile cache22.c into cache22.o
# cache22.o depends on cache22.c and relevant headers
# $<: expands to the first prerequisite (cache22.c)
$(cache22.o): cache22.c cache22.h tree.h
	$(CC) $(CFLAGS) -c $<

# Rule to compile tree.c into tree.o
# tree.o depends on tree.c and relevant headers
$(tree.o): tree.c tree.h
	$(CC) $(CFLAGS) -c $<

# Clean rule: removes all generated object files and the final executable
clean:
	rm -f $(OBJS) $(TARGET)
