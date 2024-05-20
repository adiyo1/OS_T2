
CC = gcc
CFLAGS = -Wall -g

all: ttt mync mynetcat

ttt: ttt.o
	$(CC) $(CFLAGS) ttt.o -o ttt 

ttt.o: ttt.c
	$(CC) $(CFLAGS) -c ttt.c

mync: mync.o
	$(CC) $(CFLAGS) mync.o -o mync 

mync.o: mync.c
	$(CC) $(CFLAGS) -c mync.c

mynetcat: mynetcat.o
	$(CC) $(CFLAGS) mynetcat.o -o mynetcat 

mynetcat.o: mynetcat.c
	$(CC) $(CFLAGS) -c mynetcat.c



clean:
	rm -f *.o ttt mync mynetcat