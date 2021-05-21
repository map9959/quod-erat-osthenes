# Author: Michael Paciullo

CC		= gcc

CFLAGS		= -g  -Wall -std=gnu99

CFLAGS2		= -g  -Wall

CLIBS		= -lm -pthread

all: qeo

qeo:
	$(CC) $(CFLAGS) -o qeo qeo.c $(CLIBS)

clean:
	rm -f *~ *.o a.out core qeo *.txt
