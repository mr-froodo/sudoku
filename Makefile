CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDFLAGS =
LDLIBS =

RM = rm -f


scrack: scrack.o sudoku.o
sudoku_cracker: sudoku_cracker.o

.PHONY: clean

clean:
	$(RM) sudoku_cracker sudoku_cracker.o scrack scrack.o sudoku.o