CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: jit

jit: main.o
	$(CC) $(CFLAGS) -o jit main.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f *.o jit
