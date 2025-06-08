CC=gcc
CFLAGS=-Wall -pedantic -g

all: ctar utar
ctar: ctar.o 
	$(CC) $(CFLAGS) -o ctar ctar.o
ctar.o: ctar.c
	$(CC) $(CFLAGS) -c ctar.c
utar: utar.o
	$(CC) $(CFLAGS) -o utar utar.o 
utar.o: utar.c
	$(CC) $(CFLAGS) -c utar.c

.PHONY: clean
clean:
	rm *.o ctar utar
