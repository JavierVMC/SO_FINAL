all: simulador

simulador: myFunctions.o simulador.o
	gcc simulador.o myFunctions.o -o simulador -lrt

simulador.o: simulador-2.c
	gcc -c simulador-2.c

myFunctions.o: myFunctions.c myFunctions.h
	gcc -c myFunctions.c

.PHONY: clean

clean:
	rm -f simulador *.o