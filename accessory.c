//
//  accessory.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

#define PORT 12345

Accessory myInfo;

void addrInit(struct sockaddr_in *address, int port);

int main(int argc, const char * argv[]) {
    int socketFD;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);

    puts("\n# Inizio del programma\n");

    if (argc > 1) {
        strcpy(myInfo.name, argv[1]);
        myInfo.status = -1;
        printf("Hi! I'm %s\n", myInfo.name);
    } else {
        puts("Usage: ./accessory AccessoryName\n");
        exit(EXIT_FAILURE);
    }

    puts("<CLIENT> in esecuzione...");
    addrInit(&serverAddr, PORT);

    if ((socketFD = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (connect(socketFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    getsockname(socketFD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);
    printf("<CLIENT> Connessione stabilita - Porta server: %d - Porta locale: %d\n", PORT, ntohs(clientAddr.sin_port));

    send(socketFD, myInfo.name, sizeof(myInfo.name), 0);
    recv(socketFD, &myInfo.status, sizeof(myInfo.status), 0);
    printf("<CLIENT> Risposta del server: %d\n", (int) myInfo.status);

    close(socketFD);
    puts("\n# Fine del programma\n");
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, int port) {
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    inet_aton("127.0.0.1", &address->sin_addr);
}