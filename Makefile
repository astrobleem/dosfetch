# Makefile for DOS target (using Open Watcom 1.9) hosted on Linux.
CC=wcc

# Do not enable loop optimizations. This breaks calibrate_delay() because it
# optimzes away at least some of the loops.
CFLAGS=-q -0 -za99 -aa -wx -ox -oh

all: neofetch.exe

neofetch.exe: main.o
	wlink system dos file main name neofetch

main.o: main.c inline.h
	$(CC) $(CFLAGS) main.c

clean:
	rm -f main.o neofetch.exe

install: neofetch.exe
	cp neofetch.exe /media/1TB

