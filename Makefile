CC = gcc
# -03 aggressive optimisation, -fopenmp to use <omp.h> (parrallel prossecing)
CFLAGS = -O3 -fopenmp -Wall -Wextra -std=c11 -g
SRC = main.c mt19937ar.c rabbitsim.c
OBJ = $(SRC:.c=.o)
DEPS = mt19937ar.h rabbitsim.h
EXEC = sim

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
