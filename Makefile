BUILD := build
PROG := $(BUILD)/quickforth
SRC := $(wildcard src/*.c)
HEADERS := $(wildcard src/*.h)

CFLAGS := -std=c89 -Wall -O3 -Wpedantic
CC := gcc

LIB_READLINE := $(shell pkg-config --libs readline 2>/dev/null)
ifneq (,$(LIB_READLINE))
	CFLAGS += $(LIB_READLINE) -DHAS_READLINE
endif

$(PROG): $(SRC) $(HEADERS)
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(SRC) -o $(PROG)

run: $(PROG)
	exec $(PROG)

.PHONY: run
