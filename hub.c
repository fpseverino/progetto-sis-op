//
//  hub.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

struct sockaddr_in clientAddr;

void addrInit(struct sockaddr_in *address, long IPaddr, int port);
void * threadHandler(void * clientSocket);
void interruptionHandler(int sig);
void initHome();
int joinHomeThreads();
bool checkName(char * newName); // Checks if a name is in the home array

Accessory home[MAX_ACCESSORIES];
int homeIndex;
pthread_t homeTIDs[MAX_ACCESSORIES];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int main() {
    int socketFD, newSocketFD;
    struct sockaddr_in serverAddr;
    int clientLen = sizeof(clientAddr);
    pthread_t tid;
    int semID;
    key_t key = ftok(".", 'x');

    puts("\n# Inizio del programma (hub)\n");

    initHome();

    if (signal(SIGINT, interruptionHandler) != 0) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

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

    semID = semget(key, 1, IPC_CREAT /*| IPC_EXCL*/ | 0666);
    if (semID < 0) {
        perror("semget hub");
        exit(EXIT_FAILURE);
    }
    if (initSem(semID) < 0) {
        perror("initSem");
        exit(EXIT_FAILURE);
    }
    printf("<SERVER> Allocato semaforo con ID: %d\n", semID);

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
    }

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_ACCESSORIES; i++) {
        home[i].status = DELETED;
    }
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    joinHomeThreads();

    if (deallocateSem(semID) >= 0)
        printf("<CLIENT> Deallocato semaforo con ID: %d\n", semID);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    close(socketFD);
    close(newSocketFD);
    puts("\n# Fine del programma (hub)\n");
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
    Accessory tempInfo; // case 7: checks if accessory status was updated before broadcast
    bool wasFound = false; // case 2
    int notFound = -1; // case 2
    int myIndex; // case 7: keeps the array index of the accessory to update
    bool OKtoConnect = true; // case 1

    printf("<Thread> Gestisco connessione - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));
    while (packet.request != EXIT_MENU) {
        recv(newSocketFD, &packet, sizeof(Packet), 0);
        printf("\t<Thread> Richiesta ricevuta: %d\n", packet.request);
        switch (packet.request) {
        case 1:
            // Ask permission to add accessory
            pthread_mutex_lock(&mutex);
            OKtoConnect = true;
            if (homeIndex >= MAX_ACCESSORIES) {
                puts("\t\t<Thread> Numero massimo dispositivi raggiunto");
                OKtoConnect = false;
            }
            if (!checkName(packet.accessory.name)) {
                puts("\t\t<Thread> Nome occupato");
                OKtoConnect = false;
            }
            send(newSocketFD, &OKtoConnect, sizeof(bool), 0);
            pthread_mutex_unlock(&mutex);
            break;
        case 2:
            // Read status of one accessory
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < MAX_ACCESSORIES; i++) {
                if (strcmp(packet.accessory.name, home[i].name) == 0) {
                    wasFound = true;
                    send(newSocketFD, &home[i].status, sizeof(home[i].status), 0);
                    break;
                }
            }
            if (!wasFound)
                send(newSocketFD, &notFound, sizeof(int), 0);
            pthread_mutex_unlock(&mutex);
            break;
        case 3:
            // Read status of all accessories
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < MAX_ACCESSORIES; i++) {
                send(newSocketFD, &home[i].name, sizeof(home[i].name), 0);
                send(newSocketFD, &home[i].status, sizeof(home[i].status), 0);
            }
            pthread_mutex_unlock(&mutex);
            break;
        case 4:
            // Update status of one accessory
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < MAX_ACCESSORIES; i++) {
                if (strcmp(packet.accessory.name, home[i].name) == 0) {
                    home[i].status = packet.accessory.status;
                    pthread_cond_broadcast(&cond);
                    printf("\t\t<UPDATE Thread> %s impostato a %d\n", home[i].name, home[i].status);
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            break;
        case 5:
            // Delete all accessories
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < MAX_ACCESSORIES; i++) {
                home[i].status = DELETED;
            }
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
            joinHomeThreads();
            pthread_mutex_lock(&mutex);
            initHome();
            pthread_mutex_unlock(&mutex);
            break;
        case 7:
            // Add accessory to hub
            pthread_mutex_lock(&mutex);
            myIndex = homeIndex++;
            strcpy(home[myIndex].name, packet.accessory.name);
            home[myIndex].status = 0;
            homeTIDs[myIndex] = pthread_self();
            printf("\t\t<ADD Thread> Aggiunto %s\n", home[myIndex].name);
            strcpy(tempInfo.name, home[myIndex].name);
            tempInfo.status = home[myIndex].status;
            while (true) {
                pthread_cond_wait(&cond, &mutex);
                if (tempInfo.status != home[myIndex].status) {
                    send(newSocketFD, &home[myIndex], sizeof(home[myIndex]), 0);
                    tempInfo.status = home[myIndex].status;
                    printf("\t\t<ADD Thread> Inviato aggiornamento (%d) all'accessorio %s\n", tempInfo.status, tempInfo.name);
                    if (home[myIndex].status == DELETED) {
                        printf("\t\t<ADD Thread> %s eliminato\n", tempInfo.name);
                        pthread_mutex_unlock(&mutex);
                        if (close(newSocketFD) == 0)
                            printf("<Thread> Connessione terminata - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));
                        pthread_exit(EXIT_SUCCESS);
                    }
                }
            }
            break;
        case EXIT_MENU:
            // Exit
            puts("\t\t<Thread> Uscita");
            break;
        default:
            puts("\t\t<Thread> Richiesta non valida");
            break;
        }
    }

    if (close(newSocketFD) == 0)
        printf("<Thread> Connessione terminata - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));
    pthread_exit(EXIT_SUCCESS);
}

void interruptionHandler(int sig) {

}

void initHome() {
    for (int i = 0; i < MAX_ACCESSORIES; i++) {
        strcpy(home[i].name, "");
        home[i].status = -1;
        homeTIDs[i] = (pthread_t) -1;
    }
    homeIndex = 0;
}

int joinHomeThreads() {
    for (int i = 0; i < MAX_ACCESSORIES; i++) {
        if (homeTIDs[i] >= 0) {
            if (pthread_join(homeTIDs[i], NULL) != 0) {
                perror("pthread_join");
                return 1;
            }
        }
    }
    return 0; 
}

bool checkName(char * newName) {
    for (int i = 0; i < MAX_ACCESSORIES; i++) {
        if (strcmp(newName, home[i].name) == 0)
            return false;
    }
    return true;
}