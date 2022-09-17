OBJECTS =	aesdsocket.o

CC = gcc

INCLUDES = -I.

LDFLAGS = -g -Wall

LIBS = -lm

all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@

.c.o:
	$(CC) $(LDFLAGS) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean :
	echo *.o *.i *.s *~ \#*# core .#* .new* aesdsocket
	rm -f *.o *.i *.s *~ \#*# core .#* .new* aesdsocket
