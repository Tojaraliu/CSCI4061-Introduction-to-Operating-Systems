# Makefile for Project 1 4061
# The program need to skip this part
#This test is testing make multiple files with no dependent file

all: make4061

make4061: file8.a main.o
	gcc -o make4061 file8.a main.o
main.o: main.c
	gcc -c main.c
file1.a: file2.a
	ar rcs file1.a file2.a
file2.a: file8.o file1.a
	ar rcs file2.a file3.o file1.a
file3.o: file3.c
	gcc -c file3.c

clean:
	rm *.o *.a make4061
