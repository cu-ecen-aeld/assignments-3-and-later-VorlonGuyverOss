INCLUDE_DIRS =
LIB_DIRS =

CC = $(CROSS_COMPILE)gcc

CDEFS=
CFLAGS= -O0 -g -Wall $(INCLUDE_DIRS) $(CDEFS)
LIBS=

TARGET= writer

HFILES=
CFILES= writer.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${TARGET}

writer:	writer.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ writer.o $(LIBS)

clean:
	rm -f *.o
	rm -f ${TARGET}

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
