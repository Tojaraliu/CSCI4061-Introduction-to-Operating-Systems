all: run

run: main.c dag.o
	gcc -o run main.c dag.o

dag.o: dag.c dag.h
	gcc -c dag.c dag.h