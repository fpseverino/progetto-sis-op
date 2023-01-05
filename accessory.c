//
//  accessory.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

int main(int argc, const char * argv[]) {
    int socketFD, shmID, printSemID;
    unsigned short * portSHM; // Shared memory sharing port number
    struct sockaddr_in serverAddr, clientAddr;
    int clientLen = sizeof(clientAddr);
    Accessory myInfo;
    Packet packet;

    check(shmID = shmget(ftok(".", 'x'), sizeof(unsigned short), 0666), "shmget");
    portSHM = (unsigned short *) shmat(shmID, NULL, 0);
    if (portSHM == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // System V semaphore for printing
    check(printSemID = semget(ftok(".", getppid()), 0, 0), "semget accessory");

    strcpy(myInfo.name, argv[1]);
    strcpy(packet.accessory.name, argv[1]);
    packet.request = 7;

    addrInitClient(&serverAddr, *portSHM);
    check(socketFD = socket(PF_INET, SOCK_STREAM, 0), "socket");
    check(connect(socketFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)), "connect");
    getsockname(socketFD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);

    send(socketFD, &packet, sizeof(packet), 0);

    waitSem(printSemID);
    printf("\t<%s> Connessione stabilita - Porta server: %d - Porta locale: %d\n", myInfo.name, *portSHM, ntohs(clientAddr.sin_port));
    signalSem(printSemID);
    
    while (true) {
        recv(socketFD, &myInfo, sizeof(myInfo), 0);
        waitSem(printSemID);
        if (myInfo.status == DELETED) {
            printf("\t<%s> Eliminato\n", myInfo.name);
            signalSem(printSemID);
            break;
        } else {
            printf("\t<%s> Nuovo status: %d\n", myInfo.name, myInfo.status);
        }
        signalSem(printSemID);
    }

    check(shmdt((void *) portSHM), "shmdt");
    close(socketFD);
    exit(EXIT_SUCCESS);
}