BUILD := build
PROG := $(BUILD)/quickforth
SRC := $(wildcard src/*.c)

CFLAGS := -std=c89 -Wall
CC := gcc

LIB_READLINE := $(shell pkg-config --libs readline 2>/dev/null)
ifneq (,$(LIB_READLINE))
	CFLAGS += $(LIB_READLINE) -DHAS_READLINE
endif

$(PROG): $(SRC)
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(SRC) -o $(PROG)

run: $(PROG)
	exec $(PROG)

.PHONY: run
