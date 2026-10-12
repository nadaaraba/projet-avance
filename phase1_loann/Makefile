CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I. -I../phase1_nada
LDFLAGS = 

EXEC = test_phase1_3

SRCS = main_test_loann.c \
       base64.c \
       other_base64.c \
       tests_base64.c

OBJ_DIR = build
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(EXEC)
	./$(EXEC)

clean:
	rm -rf $(OBJ_DIR)

mrproper: clean
	rm -f $(EXEC)

.PHONY: all test clean mrproper