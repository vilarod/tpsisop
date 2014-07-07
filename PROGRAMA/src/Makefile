INCLUDE:= -L/usr/lib/commons.so
all: programa
 
clean:
	rm *.o 
 
PROGRAMA.o :PROGRAMA.c 
	gcc PROGRAMA.c -c $(LIB)
 
 
programa: PROGRAMA.o  
	gcc PROGRAMA.o -o programa