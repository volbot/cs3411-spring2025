CC=gcc
CFLAGS=-Wall -pedantic -g
OBJECTS=f.o main.o

all: main
main: $(OBJECTS)
	$(CC) $(CFLAGS) -o main $(OBJECTS)
main.o: main.c
	$(CC) $(CFLAGS) -c main.c
f.o: f.c
	$(CC) $(CFLAGS) -c f.c

.PHONY: clean
clean:
	rm *.o main
