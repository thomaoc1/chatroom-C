#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "common.h"

struct clients {
        char* id;
        int socket;
    } client;

void *receiving_msg(void *client) {
    
    // Convert socket back to int
    struct clients client_data = *(struct clients *)client;
    int clientfd = client_data.socket;

    // Time and timestamp
    time_t rawtime;
    char timestamp[11];  

    // Message buffer
    char *buffer;
    size_t nbytes = 1;

    // If nbytes == 0 then socket has been disconnected
    while(nbytes > 0) {

        nbytes = receive(clientfd, (void **)&buffer, &rawtime);
        time(&rawtime);
        struct tm *local = localtime(&rawtime);
        sprintf(timestamp, "(%02d:%02d:%02d)", local->tm_hour, local->tm_min, local->tm_sec);

        if (nbytes > 0) {
            // Displays pseudo and message
            printf("%s %s", timestamp, buffer);
            free(buffer);
        }
    }
    printf("Lost connection to server\n");
    exit(0);
}

void *sending_msg(void *client) {

    // Should be ok
    time_t rawtime;
    time(&rawtime);

    // Convert socket back to int
    struct clients client_data = *(struct clients *)client;
    int clientfd = client_data.socket;
    char *pseudo = client_data.id;

    // Message buffer
    char buffer[1024]; // Message read from stdin
    char message[2048]; // Pseudo followed my message 

    while (fgets(buffer, 1024, stdin)) {
        sprintf(message, "%s: %s", pseudo, buffer);
        ssend(clientfd, message, strlen(message) + 1, rawtime);
    }

    // Terminate thread once client is done sending messages
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

    // Checking correct usage
    if(argc != 4) {
        printf("Usage: ./client <pseudo> <ip_server> <port>\n");
        exit(0);
    }

    // Parsing arguments
    char *pseudo = argv[1];
    char *ip = argv[2];
    int port = atoi(argv[3]);

    // Initialising client file descriptor
    int clientfd = checked(socket(AF_INET, SOCK_STREAM, 0));

    // Server data
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Converting IP given to binary
    checked(inet_pton(AF_INET, ip, &server.sin_addr));

    // Attempt connection to server
    checked(connect(clientfd, (struct sockaddr *)&server, sizeof(server)));
    
    struct clients client;
    client.id = pseudo;
    client.socket = clientfd;

    // Declaration and creation of threads in charge of sending/receiving messages
    pthread_t send_msg, recv_msg;
    if (pthread_create(&send_msg, NULL, sending_msg, (void *)&client) < 0) {
        perror("Thread creation failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&recv_msg, NULL, receiving_msg, (void *)&client) < 0) {
        perror("Thread creation failed\n");
        exit(EXIT_FAILURE);
    }

    // Wait for user to stop sending messages
    pthread_join(send_msg, NULL);

    // Close socket
    close(clientfd);

    return 0;
}