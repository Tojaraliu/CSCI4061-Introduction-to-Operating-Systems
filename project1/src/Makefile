all: make4061

main.o: main.c util.h
	gcc -c main.c

util.o: util.c util.h
	gcc -c util.c

make4061: main.o util.o
	gcc -o make4061 main.o util.o

clean:
	rm -Rf *.o make4061
