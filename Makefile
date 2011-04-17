#James Alexander
#Project: comp3520 Assignment2 
#Date: 2011

CC = gcc
CFLAGS = -Wall -pedantic -W -std=c99 -g -O0
OBJECTS = tutshell.o
BINARIES = tutshell
INCLUDES = tutshell.c

main: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(BINARIES)

$(OBJECTS): $(INCLUDES)

clean:
	-rm -r *.o $(BINARIES)

.PHONY: clean
