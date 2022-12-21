//
//  hub.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

#define PORT 12345
#define MAX_CONN 8
#define MAX_ACCESSORIES 5

struct sockaddr_in clientAddr;

void addrInit(struct sockaddr_in *address, long IPaddr, int port);
void * threadHandler(void * clientSocket);
void homeInit();

Accessory home[MAX_ACCESSORIES];
int homeIndex = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

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

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
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
    printf("\t<Thread> Richiesta ricevuta: %d\n", packet.request);
    switch (packet.request) {
    case 1:
        // Add accessory
        pthread_mutex_lock(&mutex);
        puts("\t<ADD Thread> Mutex bloccato");
        if (homeIndex == MAX_ACCESSORIES) {
            puts("\t<ADD Thread> Numero massimo dispositivi raggiunto");
            break;
        }
        int myIndex = homeIndex++;
        strcpy(home[myIndex].name, packet.accessory.name);
        home[myIndex].status = 0;
        printf("\t<ADD Thread> Aggiunto %s\n", home[myIndex].name);
        Accessory tempInfo;
        strcpy(tempInfo.name, home[myIndex].name);
        tempInfo.status = home[myIndex].status;
        while (true) {
            pthread_cond_wait(&cond, &mutex);
            if (tempInfo.status != home[myIndex].status) {
                send(newSocketFD, &home[myIndex], sizeof(home[myIndex]), 0);
                tempInfo.status = home[myIndex].status;
                puts("\t<ADD Thread> Inviato aggiornamento all'accessorio");
            }
        }
        pthread_mutex_unlock(&mutex);
        puts("\t<ADD Thread> Mutex sbloccato");
        break;
    case 2:
        // Read status of one accessory
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < MAX_ACCESSORIES; i++) {
            if (strcmp(packet.accessory.name, home[i].name) == 0)
                send(newSocketFD, &home[i].status, sizeof(home[i].status), 0);
        }
        pthread_mutex_unlock(&mutex);
        break;
    case 3:
        // Read status of all accessories
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < MAX_ACCESSORIES; i++) {
            printf("%s: %d\n", home[i].name, home[i].status);
        }
        pthread_mutex_unlock(&mutex);
        break;
    case 4:
        // Update status of one accessory
        pthread_mutex_lock(&mutex);
        puts("\t<UPDATE Thread> Mutex bloccato");
        for (int i = 0; i < MAX_ACCESSORIES; i++) {
            if (strcmp(packet.accessory.name, home[i].name) == 0) {
                home[i].status = packet.accessory.status;
                pthread_cond_broadcast(&cond);
                puts("\t<UPDATE Thread> Variabile condizione segnalata");
                printf("\t<UPDATE Thread> %s impostato a %d\n", home[i].name, home[i].status);
            }
        }
        pthread_mutex_unlock(&mutex);
        puts("\t<UPDATE Thread> Mutex sbloccato");
        break;
    default:
        puts("\t<Thread> Richiesta non valida");
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