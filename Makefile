#
#	Makefile for Microshell
#	JohnHenry Ward, CSCI 347 Spring 2020
#

CC = gcc
CFLAGS = -g -c -Wall
oFILES = ush.o expand.o builtin.o strmode.o
cFILES = ush.c expand.c builtin.c

ush: $(oFILES)
	$(CC) -g -o ush $(oFILES)

ush.o: ush.c
	$(CC) $(CFLAGS) ush.c

expand.o: expand.c
	$(CC) $(CFLAGS) expand.c

builtin.o: builtin.c
	$(CC) $(CFLAGS) builtin.c

strmode.o: strmode.c
	$(CC) $(CFLAGS) strmode.c


clean:
	rm -f $(oFILES)


#dependencies
cFILES: defn.h globals.h
