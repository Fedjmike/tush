CC = gcc
CFLAGS = -std=c11 -Werror -Wall -Wextra -I../libkiss -g

HEADERS = $(wildcard *.h)
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

*.o: $(HEADERS)

sh: $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

test: sh
	valgrind -q --leak-check=full ./sh

clean:
	rm -f *.o sh{,.exe}

.PHONY: test clean
