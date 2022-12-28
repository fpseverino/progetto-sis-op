//
//  hub.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

pthread_t threadPool[THREAD_POOL_SIZE];
pthread_mutex_t threadPoolMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t threadPoolCond = PTHREAD_COND_INITIALIZER;

sem_t * readSem;
pthread_mutex_t readWriteMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t updateCond = PTHREAD_COND_INITIALIZER;
int readCount = 0;

Accessory home[MAX_ACCESSORIES];
int homeIndex;
pthread_t homeTIDs[MAX_ACCESSORIES];

void addrInit(struct sockaddr_in *address, long IPaddr, int port);
void * threadHandler(void * arg); // Handler of the threads in the pool
void requestHandler(int * clientSocket); // Called inside the thread handler
void initHome();
void joinHomeThreads();
bool checkName(char * newName); // Checks if a name is in the home array

int main() {
    int newSocketFD, clientLen;
    struct sockaddr_in serverAddr, clientAddr;

    puts("\n# Inizio del programma (hub)\n");

    int semID = semget(ftok(".", 'x'), 1, IPC_CREAT /*| IPC_EXCL*/ | 0666);
    check(semID, "semget hub");
    check(initSem(semID), "initSem");
    printf("<SERVER> Allocato semaforo con ID: %d\n", semID);

    readSem = sem_open("progSisOpR", O_CREAT /*| O_EXCL*/, 0666, 1);
    check(readSem, "sem_open");

    initHome();

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&threadPool[i], NULL, threadHandler, NULL);
    }

    addrInit(&serverAddr, INADDR_ANY, PORT);
    int socketFD = socket(PF_INET, SOCK_STREAM, 0);
    check(socketFD, "socket");
    check(bind(socketFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)), "bind");
    check(listen(socketFD, SERVER_BACKLOG), "listen");
    printf("<SERVER> in attesa di connessione sulla porta: %d\n", ntohs(serverAddr.sin_port));

    while (true) {
        clientLen = sizeof(clientAddr);
        check(newSocketFD = accept(socketFD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen), "accept");
        printf("<SERVER> Connessione accettata - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));

        int * pClient = malloc(sizeof(int));
        *pClient = newSocketFD;
        pthread_mutex_lock(&threadPoolMutex);
        enqueue(pClient);
        pthread_cond_signal(&threadPoolCond);
        pthread_mutex_unlock(&threadPoolMutex);
    }

    pthread_mutex_lock(&readWriteMutex);
    for (int i = 0; i < MAX_ACCESSORIES; i++) {
        home[i].status = DELETED;
    }
    pthread_cond_broadcast(&updateCond);
    pthread_mutex_unlock(&readWriteMutex);

    joinHomeThreads();

    deallocateSem(semID);

    pthread_mutex_destroy(&threadPoolMutex);
    pthread_cond_destroy(&threadPoolCond);

    check(sem_close(readSem), "sem_close");
    check(sem_unlink("progSisOpR"), "sem_unlink");

    pthread_mutex_destroy(&readWriteMutex);
    pthread_cond_destroy(&updateCond);

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

void * threadHandler(void * arg) {
    while (true) {
        int * pClient;
        pthread_mutex_lock(&threadPoolMutex);
        if ((pClient = dequeue()) == NULL) {
            pthread_cond_wait(&threadPoolCond, &threadPoolMutex);
            pClient = dequeue();
        }
        pthread_mutex_unlock(&threadPoolMutex);
        if (pClient != NULL) {
            requestHandler(pClient);
        }
    }
}

void requestHandler(int * clientSocket) {
    int newSocketFD = * clientSocket;
    Packet packet;
    Accessory tempInfo; // case 7: checks if accessory status was updated before broadcast
    bool wasFound = false; // case 2
    int notFound = -1; // case 2
    int myIndex; // case 7: keeps the array index of the accessory to update
    bool OKtoConnect = true; // case 1

    printf("<Thread> Gestisco connessione\n");
    while (packet.request != EXIT_MENU) {
        recv(newSocketFD, &packet, sizeof(Packet), 0);
        printf("\t<Thread> Richiesta ricevuta: %d\n", packet.request);
        switch (packet.request) {
        case 1:
            // Ask permission to add accessory
            sem_wait(readSem);
            readCount++;
            if (readCount == 1)
                pthread_mutex_lock(&readWriteMutex);
            sem_post(readSem);

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

            sem_wait(readSem);
            readCount--;
            if (readCount == 0)
                pthread_mutex_unlock(&readWriteMutex);
            sem_post(readSem);
            break;
        case 2:
            // Read status of one accessory
            sem_wait(readSem);
            readCount++;
            if (readCount == 1)
                pthread_mutex_lock(&readWriteMutex);
            sem_post(readSem);

            for (int i = 0; i < MAX_ACCESSORIES; i++) {
                if (strcmp(packet.accessory.name, home[i].name) == 0) {
                    wasFound = true;
                    send(newSocketFD, &home[i].status, sizeof(home[i].status), 0);
                    break;
                }
            }
            if (!wasFound)
                send(newSocketFD, &notFound, sizeof(int), 0);
            
            sem_wait(readSem);
            readCount--;
            if (readCount == 0)
                pthread_mutex_unlock(&readWriteMutex);
            sem_post(readSem);
            break;
        case 3:
            // Read status of all accessories
            sem_wait(readSem);
            readCount++;
            if (readCount == 1)
                pthread_mutex_lock(&readWriteMutex);
            sem_post(readSem);

            for (int i = 0; i < MAX_ACCESSORIES; i++) {
                send(newSocketFD, &home[i].name, sizeof(home[i].name), 0);
                send(newSocketFD, &home[i].status, sizeof(home[i].status), 0);
            }

            sem_wait(readSem);
            readCount--;
            if (readCount == 0)
                pthread_mutex_unlock(&readWriteMutex);
            sem_post(readSem);
            break;
        case 4:
            // Update status of one accessory
            pthread_mutex_lock(&readWriteMutex);
            for (int i = 0; i < MAX_ACCESSORIES; i++) {
                if (strcmp(packet.accessory.name, home[i].name) == 0) {
                    home[i].status = packet.accessory.status;
                    pthread_cond_broadcast(&updateCond);
                    printf("\t\t<UPDATE Thread> %s impostato a %d\n", home[i].name, home[i].status);
                    break;
                }
            }
            pthread_mutex_unlock(&readWriteMutex);
            break;
        case 5:
            // Delete all accessories
            pthread_mutex_lock(&readWriteMutex);
            for (int i = 0; i < MAX_ACCESSORIES; i++) {
                home[i].status = DELETED;
            }
            pthread_cond_broadcast(&updateCond);
            puts("BROADCAST AGGIORNAMENTO");
            pthread_mutex_unlock(&readWriteMutex);
            joinHomeThreads();
            initHome();
            break;
        case 7:
            // Add accessory to hub
            pthread_mutex_lock(&readWriteMutex);
            myIndex = homeIndex++;
            strcpy(home[myIndex].name, packet.accessory.name);
            home[myIndex].status = 0;
            homeTIDs[myIndex] = pthread_self();
            printf("\t\t<ADD Thread> Aggiunto %s\n", home[myIndex].name);
            strcpy(tempInfo.name, home[myIndex].name);
            tempInfo.status = home[myIndex].status;
            pthread_mutex_unlock(&readWriteMutex);
            
            while (true) {
                pthread_mutex_lock(&readWriteMutex);
                pthread_cond_wait(&updateCond, &readWriteMutex);
                if (tempInfo.status != home[myIndex].status) {
                    send(newSocketFD, &home[myIndex], sizeof(home[myIndex]), 0);
                    tempInfo.status = home[myIndex].status;
                    printf("\t\t<ADD Thread> Inviato aggiornamento (%d) all'accessorio %s\n", tempInfo.status, tempInfo.name);
                    if (home[myIndex].status == DELETED) {
                        printf("\t\t<ADD Thread> %s eliminato\n", tempInfo.name);
                        if (close(newSocketFD) == 0)
                            printf("<Thread> Connessione terminata\n");
                        pthread_mutex_unlock(&readWriteMutex);
                        pthread_exit(EXIT_SUCCESS);
                    }
                }
                pthread_mutex_unlock(&readWriteMutex);
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
        printf("<Thread> Connessione terminata\n");
    pthread_exit(EXIT_SUCCESS);
}

void initHome() {
    pthread_mutex_lock(&readWriteMutex);
    for (int i = 0; i < MAX_ACCESSORIES; i++) {
        strcpy(home[i].name, "");
        home[i].status = -1;
        homeTIDs[i] = (pthread_t) -1;
    }
    homeIndex = 0;
    pthread_mutex_unlock(&readWriteMutex);
}

void joinHomeThreads() {
    for (int i = 0; i < MAX_ACCESSORIES; i++)
        if ((long) homeTIDs[i] >= 0)
            if (pthread_join(homeTIDs[i], NULL) != 0)
                perror("pthread_join");
}

bool checkName(char * newName) {
    bool result = true;

    sem_wait(readSem);
    readCount++;
    if (readCount == 1)
        pthread_mutex_lock(&readWriteMutex);
    sem_post(readSem);

    for (int i = 0; i < MAX_ACCESSORIES; i++) {
        if (strcmp(newName, home[i].name) == 0)
            result = false;
    }

    sem_wait(readSem);
    readCount--;
    if (readCount == 0)
        pthread_mutex_unlock(&readWriteMutex);
    sem_post(readSem);

    return result;
}