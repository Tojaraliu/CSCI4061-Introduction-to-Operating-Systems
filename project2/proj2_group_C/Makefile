CC = gcc
TARGET = shell server
OBJECT = shell.o util.o server.o
FLAGS = -Wall -g

all: server shell

server: server.o util.o
	$(CC) $(FLAGS) -o server server.o util.o

server.o: server.c
	$(CC) $(FLAGS) -c server.c

shell: shell.o util.o
	$(CC) $(FLAGS) -o shell shell.o util.o

shell.o: shell.c
	$(CC) $(FLAGS) -c shell.c

util.o: util.c
	$(CC) $(FLAGS) -c util.c

clean:
	rm -f $(OBJECT) $(TARGET)
