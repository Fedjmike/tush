CC = gcc
CFLAGS = -std=c11 -Werror -Wall -Wextra -I../libkiss -g
LDFLAGS = -lgc -lreadline

HEADERS = $(wildcard src/*.h)
OBJECTS = $(patsubst src/%.c, obj/%.o, $(wildcard src/*.c))

all: sh

obj/:
	@mkdir -p obj

obj/%.o: src/%.c obj/ $(HEADERS)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo " [CC] $@"

sh: $(OBJECTS)
	@$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	@echo " [LD] $@"
	@du -h $@
	@echo

test: sh
	valgrind -q --leak-check=full --suppressions=boehm-gc.supp ./sh

clean:
	rm -f *.o sh{,.exe}

.PHONY: all test clean
