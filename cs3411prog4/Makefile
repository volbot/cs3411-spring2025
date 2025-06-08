CC=gcc
CFLAGS=-Wall -pedantic -g

all: step1 step2 step3 step4 step5 step6 prose

prose: prose.o 
	$(CC) $(CFLAGS) -o prose prose.o
prose.o: prose.c
	$(CC) $(CFLAGS) -c prose.c

step6: step6.o 
	$(CC) $(CFLAGS) -o step6 step6.o
step6.o: step6.c
	$(CC) $(CFLAGS) -c step6.c

step5: step5.o 
	$(CC) $(CFLAGS) -o step5 step5.o
step5.o: step5.c
	$(CC) $(CFLAGS) -c step5.c

step4: step4.o 
	$(CC) $(CFLAGS) -o step4 step4.o
step4.o: step4.c
	$(CC) $(CFLAGS) -c step4.c

step3: step3.o 
	$(CC) $(CFLAGS) -o step3 step3.o
step3.o: step3.c
	$(CC) $(CFLAGS) -c step3.c

step2: step2.o 
	$(CC) $(CFLAGS) -o step2 step2.o
step2.o: step2.c
	$(CC) $(CFLAGS) -c step2.c

step1: step1.o 
	$(CC) $(CFLAGS) -o step1 step1.o
step1.o: step1.c
	$(CC) $(CFLAGS) -c step1.c

.PHONY: clean
clean:
	rm *.o step1 step2 step3 step4 step5 step6 prose
