//
//  accessory.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

#define PORT 12345

Accessory myInfo;
Packet packet;

void addrInit(struct sockaddr_in *address, int port);

int main(int argc, const char * argv[]) {
    int socketFD;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);

    if (argc > 1) {
        strcpy(myInfo.name, argv[1]);
        strcpy(packet.accessory.name, argv[1]);
        packet.request = 1;
    } else {
        puts("<ACCESSORY> Usage: ./accessory accessoryName\n");
        exit(EXIT_FAILURE);
    }

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
    printf("<%s> Connessione stabilita - Porta server: %d - Porta locale: %d\n", myInfo.name, PORT, ntohs(clientAddr.sin_port));

    send(socketFD, &packet, sizeof(packet), 0);
    
    while (true) {
        recv(socketFD, &myInfo, sizeof(myInfo), 0);
        printf("<%s> Nuovo status: %d\n", myInfo.name, myInfo.status);
    }

    close(socketFD);
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, int port) {
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    inet_aton("127.0.0.1", &address->sin_addr);
}