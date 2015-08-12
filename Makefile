CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic
LFLAGS = -lxcb -lxcb-keysyms

all: koalawm

koalawm: koalawm.c config.h
	$(CC) $(CFLAGS) $(LFLAGS) koalawm.c -o koalawm


clean:
	rm *.o koalawm
