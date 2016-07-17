FLAGS = -Wall -g

application: application.o process.o
	gcc -o application application.o process.o

process.o: process.c process.h
	gcc $(FLAGS) -c process.c

application.o: application.c
	gcc $(FLAGS) -c application.c

clean:
	rm application application.o process.o
