CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -Wall -Wextra -pedantic
LDFLAGS =
LDLIBS =

RM = rm -f

BUILD = build

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS))

.PHONY: all test clean

all: $(BUILD)/scrack

# Brute-force solver test executable
$(BUILD)/scrack: $(BUILD)/scrack.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD)/%.o: src/%.c include/sudoku.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: test/%.c include/sudoku.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $@

test: $(BUILD)/scrack
	./$(BUILD)/scrack

clean:
	$(RM) -r $(BUILD)
