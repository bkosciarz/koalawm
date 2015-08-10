CC = gcc
CFLAGS = 
LFLAGS = -lxcb -lxcb-keysyms

all: koalawm


koalawm: koalawm.c config.h
	$(CC) $(CFLAGS) $(LFLAGS) koalawm.c -o koalawm
