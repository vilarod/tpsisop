CC :=gcc
CFLAGS=-Wall -ansi -D_XOPEN_SOURCE=500
OBJS:=/home/utnso/tp-2014-1c-garras/PROGRAMA/src/PROGRAMA.o
HDRS:=PROGRAMA.h

#ALL
all: programa

#Programa
programa: $(OBJS)
	$CC $(CFLAGS) -o Programa $(OBJS)
programa.o: PROGRAMA.c $(HDRS)
	$(CC) -c $(CFLAGS) PROGRAMA.c

#Clean
clean:
	rm programa $(OBJS)
