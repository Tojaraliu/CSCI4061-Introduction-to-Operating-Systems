                     #lalala demacia
all: makeresult

makeresult: util.a main.o
	gcc -o makeresult main.o util.a

main.o: main.c #main.c
	gcc -c main.c

util.a: parse.o cal.o
	ar rcs util.a parse.o cal.o

cal.o: cal.c
	gcc -c cal.c

parse.o: parse.c
	gcc -c parse.c

clean:
	rm -rf main.o cal.o parse.o util.a makeresult
