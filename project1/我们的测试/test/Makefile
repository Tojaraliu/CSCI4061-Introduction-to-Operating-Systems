# Program files.

main.o: main.c a.h
	gcc -c main.c 
a.o: a.c a.h
	gcc -c a.c a.h
test: main.o a.o
	gcc -o test main.o a.o
run-test: test
	./test

clean:
	rm -Rf *.o test