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
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);

    int semID = semget(ftok(".", 'x'), 0, 0);
    check(semID, "semget accessory");

    strcpy(myInfo.name, argv[1]);
    strcpy(packet.accessory.name, argv[1]);
    packet.request = 7;

    addrInit(&serverAddr, PORT);
    int socketFD = socket(PF_INET, SOCK_STREAM, 0);
    check(socketFD, "socket");
    check(connect(socketFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)), "connect");
    getsockname(socketFD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);

    send(socketFD, &packet, sizeof(packet), 0);

    waitSem(semID);
    printf("\t<%s> Connessione stabilita - Porta server: %d - Porta locale: %d\n", myInfo.name, PORT, ntohs(clientAddr.sin_port));
    signalSem(semID);
    
    while (true) {
        recv(socketFD, &myInfo, sizeof(myInfo), 0);
        waitSem(semID);
        if (myInfo.status == DELETED) {
            printf("\t<%s> Eliminato\n", myInfo.name);
            signalSem(semID);
            break;
        } else {
            printf("\t<%s> Nuovo status: %d\n", myInfo.name, myInfo.status);
        }
        signalSem(semID);
    }

    close(socketFD);
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, int port) {
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    inet_aton("127.0.0.1", &address->sin_addr);
}