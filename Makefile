CC = clang
CFLAGS = $(EXTRA_CFLAGS) -std=c11 -Werror -Wall -Wextra -I../libkiss -g
LDFLAGS = $(EXTRA_LDFLAGS) -lgc -lreadline -L../libkiss -lkiss

HEADERS = $(wildcard src/*.h)
MAIN = src/sh.c
OBJECTS = $(patsubst src/%.c, obj/%.o, $(filter-out $(MAIN), $(wildcard src/*.c)))
MAIN_OBJECT = $(patsubst src/%.c, obj/%.o, $(MAIN))

INSTALLPATH = /usr/local/bin/tush

all: sh
install: $(INSTALLPATH)

obj/:
	@mkdir -p $@

bin/:
	@mkdir -p $@

obj/%.o: src/%.c $(HEADERS)
	@echo " [CC] $@"
	@$(CC) $(CFLAGS) -c $< -o $@

sh: obj/ $(OBJECTS) $(MAIN_OBJECT)
	@echo " [LD] $@"
	@$(CC) $(OBJECTS) $(MAIN_OBJECT) $(LDFLAGS) -o $@
	@du -h $@
	@echo

clean:
	rm -f obj/*.o sh sh.exe

$(INSTALLPATH): sh
	cp sh $(INSTALLPATH)

uninstall:
	rm -f $(INSTALLPATH)

TEST_CFLAGS = $(CFLAGS) -I.
TEST_LDFLAGS = $(LDFLAGS)

TEST_HEADERS = $(wildcard tests/*.h)
TESTS = $(patsubst tests/%.c, bin/%, $(wildcard tests/test-*.c))

VALGRIND = valgrind -q --leak-check=full --suppressions=boehm-gc.supp

bin/%: tests/%.c  $(HEADERS) $(TEST_HEADERS) $(OBJECTS)
	@echo " [CC] $@"
	@$(CC) $(TEST_CFLAGS) $< $(OBJECTS) $(TEST_LDFLAGS) -o $@
	@echo " [$@]"
	@$(VALGRIND) $@
	@echo

tests: bin/ $(TESTS)

run: sh
	$(VALGRIND) ./sh

.PHONY: all tests run clean install uninstall
