//
//  hub.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

#define PORT 12345
#define MAX_CONN 8
#define BUFF_SIZE 128

struct sockaddr_in clientAddr;

void addrInit(struct sockaddr_in *address, long IPaddr, int port);
void * threadHandler(void * clientSocket);

Accessory home[5];

int main() {
    int socketFD, newSocketFD;
    int result;
    struct sockaddr_in serverAddr;
    int clientLen = sizeof(clientAddr);
    pthread_t tid;

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
    char buff[BUFF_SIZE];
    printf("<Thread> Gestisco connessione - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));
    recv(newSocketFD, buff, sizeof(buff), 0);
    printf("<Thread> Dati ricevuti: %s\n", buff);
    for (int i = 0; i < 5; i++) {
        if (strcmp(buff, home[i].name) == 0)
            send(newSocketFD, &home[i].status, sizeof(home[i].status), 0);
    }
    
    if (close(newSocketFD) == 0)
        printf("<Thread> Connessione terminata - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));
    pthread_exit(EXIT_SUCCESS);
}