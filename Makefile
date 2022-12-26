simulador: simulador.o
	gcc simulador.o -o simulador

simulador.o: simulador.c
	gcc -c simulador.c

.PHONY: clean

clean:
	rm -f simulador *.o