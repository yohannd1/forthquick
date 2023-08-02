BUILD := build
PROG := $(BUILD)/quickforth
CC := gcc
CFLAGS := -std=c89

SRC := $(wildcard src/*.c)

$(PROG): $(SRC)
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(SRC) -o $(PROG)

run: $(PROG)
	exec $(PROG)

.PHONY: run
