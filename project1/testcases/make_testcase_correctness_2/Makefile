# Makefile for Project 1 4061
# The program need to skip this part
#This test is testing make multiple files
			
######
	#


    all: make4061

make4061: file4.a file8.o main.o
	gcc -o make4061 file4.a file8.o main.o
main.o: main.c
	gcc -c main.c
file4.a: file6.o file7.a
	ar rcs file4.a file6.o file7.a
file6.o: file6.c
	gcc -c file6.c
file7.a: file8.o
	ar rcs file7.a file8.o
file8.o: file8.c
	gcc -c file8.c

clean:
	rm *.o *.a make4061
