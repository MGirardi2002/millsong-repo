# Compiler
CC = mpicc

# Compiler flags
# -Wall: Enable all warnings
# -O2: Optimization level 2
CFLAGS = -Wall -O2

# Executable name
EXEC = spotify_analyzer

# Source file
SRC = spotify_analyzer.c

# Number of processes for mpirun (default is 4)
NP ?= 4

# Default target: builds the executable
all: $(EXEC)

# Rule to build the executable from the source file
$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)

# Target to run the application
# Usage: make run
# or to specify the number of processes: make run NP=8
run: all
	@echo "Running $(EXEC) with $(NP) processes..."
	@mpirun -n $(NP) ./$(EXEC)

# Target to clean up the build files
# Removes the executable
clean:
	@echo "Cleaning up..."
	rm -f $(EXEC)

# Phony targets are not files
.PHONY: all run clean