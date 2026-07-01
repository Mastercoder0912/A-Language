CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -lm
TARGET = arun
SRCS = main.c interpreter.c lexer.c parser.c ast.c runtime.c builtins.c

.PHONY: build test clean install uninstall update

build:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

test: build
	./$(TARGET) test_full.a

clean:
	rm -f $(TARGET)

install: build
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)

uninstall:
	rm -f /usr/local/bin/$(TARGET)

update: uninstall install
