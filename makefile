FLAGS = -Wall -Wextra -Wpedantic
all: client server

client: client.c common.h
	g++ client.c $(FLAGS) -o client common.h -pthread

server: server.c common.h
	g++ server.c $(FLAGS) -o server common.h -pthread

clean:
	rm -f server client