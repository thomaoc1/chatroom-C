#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#include "common.h"

/* Globals */

// All sockets
int clients[1024];

// Threads
pthread_t tid[1024];
pthread_mutex_t mutex;

// Number of clients
static int nclients = 0;

// Server file descriptor
int serverfd;

void decrement_nclients(){
    // Lock out other threads when called
    pthread_mutex_lock(&mutex);
    nclients--;
    pthread_mutex_unlock(&mutex);
}

void increment_nclients() {
    pthread_mutex_lock(&mutex);
    nclients++;
    pthread_mutex_unlock(&mutex);
}

void sig_handler(int sig) {

    // Using '\r' to hide '^C' when used 
    printf("\rClosing server\n");

    // Closing all client sockets
    for (int i = 0; i < nclients; i++) {
        printf("- Closing socket %d\n", i);
        close(clients[i]);
    }

    // Closing the server socket 
    printf("- Closing master socket\n");
    close(serverfd);

    // Exiting the program
    exit(0);
}

void clean_array(int socket) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < nclients; i++) {
        if (clients[i] == socket) {
            clients[i] = clients[nclients - 1];
            break;
        }
    }    
    pthread_mutex_unlock(&mutex);
}

void message_send_all(int socket, const char* message, const char* pseudo) {

    char buffer[1024];
    sprintf(buffer, "%s: %s", pseudo, message);

    // Iterate through clients 
    for (int i = 0; i < nclients; i++) {

        ssend(clients[i], buffer, strlen(buffer) + 1);
        
    }

}

void alert_disconnect(int socket, const char* pseudo) {

    char buffer[1024];
    sprintf(buffer, "%s: [disconnected]", pseudo);
    for (int i = 0; i < nclients; i++) {
        
        if (clients[i] != socket) {
            ssend(clients[i], buffer, strlen(buffer) + 1);
        }
        
    }
}

void *handle_client(void *socket) {
    // Convert socket pointer to correct type
    int clientfd = *(int *)socket;

    // Client pseudo
    char* pseudo;
    size_t nbytes = receive(clientfd, (void **)&pseudo);

    // Message to be sent
    char *buffer;

    printf("'%s' has connected\n", pseudo);
    message_send_all(clientfd, "[connected]\n", pseudo);
    while (nbytes > 0) {
        nbytes = receive(clientfd, (void **)&buffer);
        if (nbytes > 0) {
            message_send_all(clientfd, buffer, pseudo);
            free(buffer);
        }
    }

    // Alert server and other clients
    printf("'%s' has disconnected\n", pseudo);
    alert_disconnect(clientfd, pseudo);

    // Replacing slot in clients array to avoid holes
    clean_array(clientfd);
    
    // Close socket
    close(clientfd);

    // Decrement number of clients
    decrement_nclients();
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

    // Verifying correct usage 
    if(argc != 2) {
        printf("Usage: ./server <port>\n");
        exit(0);
    }

    // Signal handler
    signal(SIGINT, sig_handler);

    // Port
    int port = atoi(argv[1]);

    // Creating server socket
    serverfd = checked(socket(AF_INET, SOCK_STREAM, 0));

    // Allowing resusage of port /
    int opt = 1;
    checked(setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)));

    // Server data
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    socklen_t addrlen = sizeof(address);

    // Binding socket to IP and port combination
    checked(bind(serverfd, (struct sockaddr *)&address, addrlen));

    // Listening on port with max queue of 5
    checked(listen(serverfd, 5));

    while (1) {
        // Accept incoming conncetion and save socket in clients array
        clients[nclients] = checked(accept(serverfd, (struct sockaddr *)&address, &addrlen));

        // Create thread to manage client
        if (pthread_create(&tid[nclients], NULL, handle_client, &clients[nclients]) < 0) {
            perror("Thread creation has failed\n");
            exit(EXIT_FAILURE);
        }
        increment_nclients();
    }
    
    return 0;
}