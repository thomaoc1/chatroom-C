#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
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
int serverfd = 0;
static int nclients = 0;

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


void message_send_all(const char* message, const char* pseudo) {

    char buffer[1024];
    sprintf(buffer, "%s: %s", pseudo, message);

    // Iterate through clients 
    for (int i = 0; i < nclients; i++) {
        ssend(clients[i], buffer, strlen(buffer) + 1);
    }

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

    // Declaring File desriptor set
    fd_set readfds; 

    while (1) {
        // Resetting FD set
        FD_ZERO(&readfds);

        // Adding server socket to FD set
        FD_SET(serverfd, &readfds);

        int maxfd = serverfd;

        // Re-initialising FD set using client array
        for(int i = 0; i < nclients; i++) {
            FD_SET(clients[i], &readfds);

            // ????
            if (clients[i] > maxfd) {
                maxfd = clients[i];
            }
        }

        // Select waits for a socket to be ready, once a socket has been found, all other sockets in the set
        // are removed, leaving the one that is ready
        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(serverfd, &readfds)) {
            // Server is ready, therefore there is a new connection
            clients[nclients] = checked(accept(serverfd, (struct sockaddr *)&address, &addrlen));
            printf("Client has connected\n");
            nclients++;
        } else {
            // Otherwise it's a client sending a message
            for(int i = 0; i < nclients; i++) {

                // If client is in FD set, they are sending a message
                if(FD_ISSET(clients[i], &readfds)){
                    // First thing a client sends is their pseudo                    
                    char *pseudo;
                    size_t nbytes = checked(receive(clients[i], (void **)&pseudo));
                    
                    char *buffer;
                    nbytes = checked(receive(clients[i], (void **)&buffer));
                    if(nbytes > 0) {
                        message_send_all(buffer, pseudo);
                        free(buffer);
                    } else {
                        close(clients[i]);
                        printf("Client '%s' has disconnected.\n", pseudo);
                        clients[i] = clients[nclients - 1];
                        nclients--;
                    }
                }
            }

        }        


    }
    
    return 0;
}