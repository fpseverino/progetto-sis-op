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
    int socketFD, printSemID;
    struct sockaddr_in serverAddr, clientAddr;
    int clientLen = sizeof(clientAddr);

    check(printSemID = semget(ftok(".", 'x'), 0, 0), "semget accessory");

    strcpy(myInfo.name, argv[1]);
    strcpy(packet.accessory.name, argv[1]);
    packet.request = 7;

    addrInit(&serverAddr, PORT);
    check(socketFD = socket(PF_INET, SOCK_STREAM, 0), "socket");
    check(connect(socketFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)), "connect");
    getsockname(socketFD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);

    send(socketFD, &packet, sizeof(packet), 0);

    waitSem(printSemID);
    printf("\t<%s> Connessione stabilita - Porta server: %d - Porta locale: %d\n", myInfo.name, PORT, ntohs(clientAddr.sin_port));
    signalSem(printSemID);
    
    while (true) {
        recv(socketFD, &myInfo, sizeof(myInfo), 0);
        waitSem(printSemID);
        if (myInfo.status == DELETED) {
            printf("\n\t<%s> Eliminato", myInfo.name);
            signalSem(printSemID);
            break;
        } else {
            printf("\t<%s> Nuovo status: %d\n", myInfo.name, myInfo.status);
        }
        signalSem(printSemID);
    }

    close(socketFD);
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, int port) {
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    inet_aton("127.0.0.1", &address->sin_addr);
}