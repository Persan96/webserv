
# Compiler
CC = gcc
# Linker
LD = gcc

# Directories
INC_DIR = include
OBJ_DIR = objects
SRC_DIR = src

# Included libs
CFLAGS = -I$(INC_DIR)

# All files ending with .c in the SRC_DIR
SRCS = $(wildcard $(SRC_DIR)/*.c)
# All file names in SRC_DIR ending with .c changes to .o
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Program name
PROG = webserver

# Path to rm program
RM = /bin/rm

# Create program
all: $(PROG)

# Link program
$(PROG): $(OBJS)
	$(LD) $^ -o $(PROG)

# Compile program into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(FLAGS) -c $< -o $@

# Remove object files
clean:
	$(RM) $(PROG) $(OBJS)
