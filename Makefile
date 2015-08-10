CC = gcc
CFLAGS = 
LFLAGS = -lxcb -lxcb-keysyms

all: koalawm


koalawm: koalawm.c
	$(CC) $(CFLAGS) $(LFLAGS) koalawm.c -o koalawm