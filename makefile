CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -lm -lsqlite3
SRCS = main.c interpreter.c lexer.c parser.c ast.c runtime.c builtins.c
TARGET = arun

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    INSTALL_DIR = ~/.local/bin
endif
ifeq ($(UNAME_S),Darwin)
    INSTALL_DIR = ~/.local/bin
endif
ifeq ($(OS),Windows_NT)
    INSTALL_DIR = $(APPDATA)\bin
    TARGET = arun.exe
endif

.PHONY: build install uninstall update clean

build:
	$(CC) -o $(TARGET) $(SRCS) $(CFLAGS)

install: build
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/$(TARGET)
	@echo "Installed to $(INSTALL_DIR)/$(TARGET)"

uninstall:
	rm -f $(INSTALL_DIR)/$(TARGET)
	@echo "Uninstalled from $(INSTALL_DIR)/$(TARGET)"

update: uninstall install
	@echo "Updated successfully"

clean:
	rm -f $(TARGET)
	@echo "Cleaned build artifacts"
