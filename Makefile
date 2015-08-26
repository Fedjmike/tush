CC = clang
CFLAGS = -std=c11 -O3 -Werror -Wall -Wextra -I../libkiss -g
LDFLAGS = -lgc -lreadline -L../libkiss -lkiss

HEADERS = $(wildcard src/*.h)
OBJECTS = $(patsubst src/%.c, obj/%.o, $(wildcard src/*.c))

INSTALLPATH = /usr/local/bin/tush

all: sh
install: $(INSTALLPATH)

obj/:
	@mkdir -p obj

obj/%.o: src/%.c $(HEADERS)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo " [CC] $@"

sh: obj/ $(OBJECTS)
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo " [LD] $@"
	@du -h $@
	@echo

test: sh
	valgrind -q --leak-check=full --suppressions=boehm-gc.supp ./sh

clean:
	rm -f obj/*.o sh sh.exe

$(INSTALLPATH): sh
	cp sh $(INSTALLPATH)

uninstall:
	rm -f $(INSTALLPATH)

.PHONY: all test clean install uninstall
