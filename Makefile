CC = gcc
# -03 aggressive optimisation, -fopenmp to use <omp.h> (parrallel prossecing)
CFLAGS = -O3 -fopenmp -Wall -Wextra -std=c11 -g
SRC = main.c pcg_basic.c rabbitsim.c stats.c
OBJ = $(SRC:.c=.o)
DEPS = pcg_basic.h rabbitsim.h
EXEC = sim

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ -lm

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
