

CC = gcc
LD = gcc

INC_DIR = include
OBJ_DIR = objects
SRC_DIR = src

CFLAGS = -I$(INC_DIR)

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

PROG = webserver

RM = /bin/rm

all: $(PROG)

$(PROG): $(OBJS)
	$(LD) $^ -o $(PROG)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(FLAGS) -c $< -o $@

install:
	mkdir -d include
	mkdir -d objects
	mkdir -d src

clean:
	$(RM) $(PROG) $(OBJS)
