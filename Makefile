CC = gcc
CFLAGS = -std=c11 -Werror -Wall -Wextra -Ilibfc

HEADERS = $(wildcard *.h)
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

*.o: $(HEADERS)

sh: $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

clean:
	rm *.o sh{,.exe}

.PHONY: clean
