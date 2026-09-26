CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -Iinclude
SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=obj/%.o)

bin/shell: $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj/* bin/*
