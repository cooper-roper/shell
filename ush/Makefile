# This is a very simple makefile#

CFLAGS=-g -c -Wall -O

run: ush builtin expand
	cc -g -o ush ush.o builtin.o expand.o

ush: ush.o defn.h globals.h
	$(CC) $(CFLAGS) ush.c

builtin: builtin.o defn.h globals.h
	$(CC) $(CFLAGS) builtin.c

expand: expand.o defn.h globals.h
	$(CC) $(CFLAGS) expand.c

clean:
	rm -f *.o ush builtin expand
