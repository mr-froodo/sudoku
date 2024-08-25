CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDFLAGS =
LDLIBS = -fopenmp

RM = rm -f

scrack: scrack.o sudoku.o
sudoku_cracker: sudoku_cracker.o
test_omp: test_omp.o

.PHONY: clean

clean:
	$(RM) sudoku_cracker sudoku_cracker.o test_omp test_omp.o scrack scrack.o sudoku.o