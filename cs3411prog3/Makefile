CC=gcc
CFLAGS=-Wall -pedantic -g

all: encode decode
encode: encode.o 
	$(CC) $(CFLAGS) -o encode encode.o
encode.o: encode.c
	$(CC) $(CFLAGS) -c encode.c
decode: decode.o
	$(CC) $(CFLAGS) -o decode decode.o 
decode.o: decode.c
	$(CC) $(CFLAGS) -c decode.c

.PHONY: clean
clean:
	rm *.o encode decode
