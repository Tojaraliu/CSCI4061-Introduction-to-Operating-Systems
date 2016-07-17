CC = gcc
CFLAGS = -D_REENTRANT
LDFLAGS = -lpthread
FLAGS = -Wall -g

web_server: server.o
	${CC} -o web_server_http server.o util.o ${LDFLAGS}

server.o: server.c
	${CC} ${FLAGS} -c server.c ${LDFLAGS}

clean:
	rm server.o web_server_http
