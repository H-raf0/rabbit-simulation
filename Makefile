CC = gcc
CFLAGS = -Wall -Wextra -std=c11 
SRC = main.c mt19937ar.c rabbitsim.c
OBJ = $(SRC:.c=.o)
DEPS = mt19937ar.h rabbitsim.h
EXEC = sim

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $^ -o $@

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
