//
//  accessory.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

Accessory myInfo;
Packet packet;

void addrInit(struct sockaddr_in *address, int port);

int main(int argc, const char * argv[]) {
    int socketFD;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);
    int semID;
    key_t key = ftok(".", 'x');

    semID = semget(key, 0, 0);
    if (semID < 0) {
        perror("semget accessory");
        exit(EXIT_FAILURE);
    }

    if (argc > 1) {
        strcpy(myInfo.name, argv[1]);
        strcpy(packet.accessory.name, argv[1]);
        packet.request = 7;
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

    send(socketFD, &packet, sizeof(packet), 0);

    if (waitSem(semID) != 0)
        perror("waitSem accessory");

    printf("\t<%s> Connessione stabilita - Porta server: %d - Porta locale: %d\n", myInfo.name, PORT, ntohs(clientAddr.sin_port));

    if (signalSem(semID) != 0)
        perror("signalSem accessory");
    
    while (true) {
        recv(socketFD, &myInfo, sizeof(myInfo), 0);
        if (waitSem(semID) != 0)
            perror("waitSem accessory");
        if (myInfo.status == DELETED) {
            printf("\t<%s> Eliminato\n", myInfo.name);
            break;
        } else {
            printf("\t<%s> Nuovo status: %d\n", myInfo.name, myInfo.status);
        }
        if (signalSem(semID) != 0)
            perror("signalSem accessory");
    }

    close(socketFD);
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, int port) {
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    inet_aton("127.0.0.1", &address->sin_addr);
}