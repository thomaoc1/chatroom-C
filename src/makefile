CC = g++
FLAGS = -Wall 
all: client server

client: client.c common.h
	$(CC) client.c $(FLAGS) -o client common.h -pthread

server: server.c common.h
	$(CC) server.c $(FLAGS) -o server common.h -pthread

clean:
	rm -f server client