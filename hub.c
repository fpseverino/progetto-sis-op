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

// Reader-writer semaphores
sem_t * readSem;
pthread_mutex_t readWriteMutex = PTHREAD_MUTEX_INITIALIZER;
int readCount = 0;
pthread_cond_t updateCond = PTHREAD_COND_INITIALIZER; // Broadcasts update to accessory threads

Accessory home[MAX_ACCESSORIES];
int homeIndex;
pthread_t homeTIDs[MAX_ACCESSORIES];

pthread_mutex_t tidMutex = PTHREAD_MUTEX_INITIALIZER;

bool serverIsRunning = true;

void addrInit(struct sockaddr_in *address, long IPaddr, int port);

void * threadHandler(void * arg); // Handler of the threads in the pool
void requestHandler(int * clientSocket); // Called inside the thread handler

// Reader-writer problem
void startReading();
void endReading();

void signalHandler(int sig);

void initHome();
void joinHomeThreads();
bool checkName(char * newName); // Checks if a name is in the home array

int main() {
    int socketFD, newSocketFD, clientLen, printSemID;
    struct sockaddr_in serverAddr, clientAddr;

    puts("\n# Inizio del programma (hub)\n");

    // Semaphore used by device and accessories for printing
    check(printSemID = semget(ftok(".", 'x'), 1, IPC_CREAT /*| IPC_EXCL*/ | 0666), "semget hub");
    check(initSem(printSemID, 1), "initSem");
    printf("<SERVER> Allocato semaforo System V con ID: %d\n", printSemID);

    // Semaphore used for solving reader-writer problem
    readSem = sem_open("progSisOpR", O_CREAT /*| O_EXCL*/, 0666, 1);
    if (readSem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    puts("<SERVER> Allocato semaforo POSIX -progSisOpR-");

    signal(SIGINT, signalHandler);

    initHome();

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
        pthread_create(&threadPool[i], NULL, threadHandler, NULL);

    addrInit(&serverAddr, INADDR_ANY, PORT);
    check(socketFD = socket(PF_INET, SOCK_STREAM, 0), "socket");
    check(bind(socketFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)), "bind");
    check(listen(socketFD, SERVER_BACKLOG), "listen");
    printf("<SERVER> in attesa di connessione sulla porta: %d\n", ntohs(serverAddr.sin_port));

    while (serverIsRunning) {
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
    for (int i = 0; i < MAX_ACCESSORIES; i++)
        home[i].status = DELETED;
    pthread_cond_broadcast(&updateCond);
    pthread_mutex_unlock(&readWriteMutex);
    joinHomeThreads();
    puts("<SERVER> Eliminati tutti gli accessori");

    deallocateSem(printSemID);
    puts("<SERVER> Deallocato semaforo System V");

    check(sem_close(readSem), "sem_close");
    check(sem_unlink("progSisOpR"), "sem_unlink");
    puts("<SERVER> Eliminato semaforo POSIX");

    pthread_mutex_destroy(&threadPoolMutex);
    pthread_cond_destroy(&threadPoolCond);

    pthread_mutex_destroy(&readWriteMutex);
    pthread_cond_destroy(&updateCond);

    pthread_mutex_destroy(&tidMutex);
    puts("<SERVER> Distrutti mutex e cond");

    close(socketFD);
    close(newSocketFD);
    puts("<SERVER> Socket chiuse");

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
        if (pClient != NULL)
            requestHandler(pClient);
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
            startReading();
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
            endReading();
            break;
        case 2:
            // Read status of one accessory
            startReading();
            for (int i = 0; i < MAX_ACCESSORIES; i++)
                if (strcmp(packet.accessory.name, home[i].name) == 0) {
                    wasFound = true;
                    send(newSocketFD, &home[i].status, sizeof(home[i].status), 0);
                    break;
                }
            if (!wasFound)
                send(newSocketFD, &notFound, sizeof(int), 0);
            endReading();
            break;
        case 3:
            // Read status of all accessories
            startReading();
            for (int i = 0; i < MAX_ACCESSORIES; i++) {
                send(newSocketFD, &home[i].name, sizeof(home[i].name), 0);
                send(newSocketFD, &home[i].status, sizeof(home[i].status), 0);
            }
            endReading();
            break;
        case 4:
            // Update status of one accessory
            pthread_mutex_lock(&readWriteMutex);
            for (int i = 0; i < MAX_ACCESSORIES; i++)
                if (strcmp(packet.accessory.name, home[i].name) == 0) {
                    home[i].status = packet.accessory.status;
                    pthread_cond_broadcast(&updateCond);
                    printf("\t\t<UPDATE Thread> %s impostato a %d\n", home[i].name, home[i].status);
                    break;
                }
            pthread_mutex_unlock(&readWriteMutex);
            break;
        case 5:
            // Delete all accessories
            pthread_mutex_lock(&readWriteMutex);
            for (int i = 0; i < MAX_ACCESSORIES; i++)
                home[i].status = DELETED;
            pthread_cond_broadcast(&updateCond);
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
            pthread_mutex_lock(&tidMutex);
            homeTIDs[myIndex] = pthread_self();
            pthread_mutex_unlock(&tidMutex);
            printf("\t\t<ADD Thread> Aggiunto %s\n", home[myIndex].name);
            strcpy(tempInfo.name, home[myIndex].name);
            tempInfo.status = home[myIndex].status;
            while (true) {
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

void startReading() {
    sem_wait(readSem);
    readCount++;
    if (readCount == 1)
        pthread_mutex_lock(&readWriteMutex);
    sem_post(readSem);
}

void endReading() {
    sem_wait(readSem);
    readCount--;
    if (readCount == 0)
        pthread_mutex_unlock(&readWriteMutex);
    sem_post(readSem);
}

void signalHandler(int sig) {
    serverIsRunning = false;
}

void initHome() {
    pthread_mutex_lock(&readWriteMutex);
    for (int i = 0; i < MAX_ACCESSORIES; i++) {
        strcpy(home[i].name, "");
        home[i].status = -1;
        pthread_mutex_lock(&tidMutex);
        homeTIDs[i] = (pthread_t) -1;
        pthread_mutex_unlock(&tidMutex);
    }
    homeIndex = 0;
    pthread_mutex_unlock(&readWriteMutex);
}

void joinHomeThreads() {
    pthread_mutex_lock(&tidMutex);
    for (int i = 0; i < MAX_ACCESSORIES; i++)
        if ((long) homeTIDs[i] >= 0)
            if (pthread_join(homeTIDs[i], NULL) != 0)
                perror("pthread_join");
    pthread_mutex_unlock(&tidMutex);
}

bool checkName(char * newName) {
    bool result = true;
    startReading();
    for (int i = 0; i < MAX_ACCESSORIES; i++)
        if (strcmp(newName, home[i].name) == 0)
            result = false;
    endReading();
    return result;
}