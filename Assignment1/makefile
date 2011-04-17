#James Alexander
#Project: comp3520 Assignment2 
#Date: 2011

CC = gcc
CFLAGS = -Wall
OBJECTS = myshell.o
BINARIES = myshell
INCLUDES = myshell.c

main: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(BINARIES)

$(OBJECTS): $(INCLUDES)

clean:
	-rm -r *.o $(BINARIES)

.PHONY: clean
