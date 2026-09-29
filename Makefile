CC = gcc
PYTHON ?= python3
CFLAGS = -Wall -Wextra -std=c99 -g -Iinclude
SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=obj/%.o)

.PHONY: all clean test
all: bin/shell

bin/shell: $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

test: bin/shell
	$(PYTHON) tests/test_shell.py

-include $(OBJ:.o=.d)

clean:
	rm -rf obj/* bin/*
