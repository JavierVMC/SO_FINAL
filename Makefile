all: simulador server

simulador: simulador.o myFunctions.o
	gcc simulador.o myFunctions.o -o simulador

server: server.o
	gcc server.o -o server

simulador.o: simulador.c
	gcc -c simulador.c

myFunctions.o: myFunctions.c
	gcc -c myFunctions.c

.PHONY: clean

clean:
	rm -f simulador *.o