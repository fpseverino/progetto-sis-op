//
//  hub.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

#define PORT 12345
#define MAX_CONN 8
#define MAX_ACCESSORIES 8

struct sockaddr_in clientAddr;

void addrInit(struct sockaddr_in *address, long IPaddr, int port);
void * threadHandler(void * clientSocket);
void homeInit();

Accessory home[MAX_ACCESSORIES];
int homeIndex = 0;

int main() {
    int socketFD, newSocketFD;
    struct sockaddr_in serverAddr;
    int clientLen = sizeof(clientAddr);
    pthread_t tid;

    //homeInit();

    puts("\n# Inizio del programma\n");
    puts("<SERVER> in esecuzione...");
    addrInit(&serverAddr, INADDR_ANY, PORT);

    if ((socketFD = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (bind(socketFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(socketFD, MAX_CONN) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("<SERVER> in attesa di connessione sulla porta: %d\n", ntohs(serverAddr.sin_port));

    while (true) {
        if ((newSocketFD = accept(socketFD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen)) == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("<SERVER> Connessione accettata - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));

        if (pthread_create(&tid, NULL, threadHandler, (void *) &newSocketFD) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
        puts("<SERVER> Thread generato");
    }

    close(socketFD);
    close(newSocketFD);
    puts("\n# Fine del programma\n");
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, long IPaddr, int port) {
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(IPaddr);
    address->sin_port = htons(port);
}

void * threadHandler(void * clientSocket) {
    int newSocketFD = * (int *) clientSocket;
    Packet packet;
    printf("\t<Thread> Gestisco connessione - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));
    recv(newSocketFD, &packet, sizeof(Packet), 0);
    printf("\t<Thread> Richiesta ricevuta: %d - Nome accessorio: %s\n", packet.request, packet.accessory.name);
    switch (packet.request) {
    case 1:
        for (int i = 0; i < 5; i++) {
            if (strcmp(packet.accessory.name, home[i].name) == 0)
                send(newSocketFD, &home[i].status, sizeof(home[i].status), 0);
        }
        break;
    case 2:
        strcpy(home[homeIndex].name, packet.accessory.name);
        home[homeIndex].status = 0;
        homeIndex++;
        break;
    default:
        break;
    }
    
    if (close(newSocketFD) == 0)
        printf("\t<Thread> Connessione terminata - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));
    pthread_exit(EXIT_SUCCESS);
}

void homeInit() {
    strcpy(home[0].name, "Luce1");
    home[0].status = 0;
    strcpy(home[1].name, "Luce2");
    home[1].status = 1;
    strcpy(home[2].name, "Termostato");
    home[2].status = 21;
    strcpy(home[3].name, "Luce3");
    home[3].status = 1;
    strcpy(home[4].name, "Luce4");
    home[4].status = 0;
}