//
//  hub.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

Accessory home[MAX_ACCESSORIES]; // Hold name and status of all accesories connected
int homeIndex; // Current index of home

unsigned short * portSHM; // Shared memory sharing port number

int msgID; // Messages queue sending requests to thread pool

pthread_t threadPool[THREAD_POOL_SIZE];
pthread_t acceptConnectionTID; // Current thread running acceptConnection

sem_t * deleteSem; // Semaphore for waiting conclusion of accessories
pthread_mutex_t deleteMutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to exclude other threads from waiting on deleteSem

// Semaphores used for solving producer-consumer problem
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
sem_t * emptyQueueSem;
sem_t * fullQueueSem;

// Mutexes used for solving reader-writer problem
pthread_mutex_t readMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readWriteMutex = PTHREAD_MUTEX_INITIALIZER;
int readCount = 0;
pthread_cond_t updateCond = PTHREAD_COND_INITIALIZER; // Broadcasts update to accessory threads

struct sockaddr_in clientAddr;
int socketFD, newSocketFD, clientLen;

void initHome();
bool checkName(char * newName); // Checks if a name is in the home array

void signalHandler(int sig); // Handles ^C to properly close the server

void * acceptConnection(void * arg); // Thread accepting incoming connection, can be canceled
void * threadFunction(void * arg); // Function of the threads in the pool
void requestHandler(int newSocketFD); // Called inside the thread handler

// Reader-writer problem
void startReading();
void endReading();

void addrInitServer(struct sockaddr_in *address, long IPaddr, int port);

int main(int argc, const char * argv[]) {
    int shmID;
    unsigned short port;
    unsigned long longPort;
    char * endPtr;
    struct sigaction act;
    struct sockaddr_in serverAddr;

    puts("\n# Inizio del programma (hub)\n");

    // Get port number from command line
    if (argc <= 1) {
        puts("<SERVER> Usage: ./hub PORT");
        exit(EXIT_FAILURE);
    }
    longPort = strtoul(argv[1], &endPtr, 10);
    if (endPtr == argv[1]) {
        puts("<SERVER> ERRORE: Nessuna cifra trovata");
        exit(EXIT_FAILURE);
    } else if (longPort > USHRT_MAX) {
        puts("<SERVER> ERRORE: Valore troppo grande");
        exit(EXIT_FAILURE);
    } else {
        port = (unsigned short) longPort;
    }

    initHome();

    act.sa_handler = signalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    check(sigaction(SIGINT, &act, NULL), "sigaction");

    check(shmID = shmget(ftok(".", 'x'), sizeof(unsigned short), IPC_CREAT | 0666), "shmget");
    portSHM = (unsigned short *) shmat(shmID, NULL, 0);
    if (portSHM == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    *portSHM = port;
    printf("<SERVER> Creata shared memory con ID: %d\n", shmID);

    check(msgID = msgget(IPC_PRIVATE, IPC_CREAT | 0666), "msgget");
    printf("<SERVER> Creata coda di messaggi con ID: %d\n", msgID);

    deleteSem = sem_open("progSisOpD", O_CREAT /*| O_EXCL*/, 0666, 0);
    if (deleteSem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    emptyQueueSem = sem_open("progSisOpE", O_CREAT /*| O_EXCL*/, 0666, MAX_REQUEST);
    if (emptyQueueSem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    fullQueueSem = sem_open("progSisOpF", O_CREAT /*| O_EXCL*/, 0666, 0);
    if (fullQueueSem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    puts("<SERVER> Allocati semafori POSIX 'progSisOpD', 'progSisOpE' e 'progSisOpF'");

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
        pthread_create(&threadPool[i], NULL, threadFunction, NULL);

    addrInitServer(&serverAddr, INADDR_ANY, *portSHM);
    check(socketFD = socket(PF_INET, SOCK_STREAM, 0), "socket");
    check(bind(socketFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)), "bind");
    check(listen(socketFD, SERVER_BACKLOG), "listen");
    printf("<SERVER> in attesa di connessione sulla porta: %d\n", ntohs(serverAddr.sin_port));

    // Main cicle
    pthread_create(&acceptConnectionTID, NULL, acceptConnection, NULL);
    pthread_join(acceptConnectionTID, NULL);

    pthread_mutex_lock(&readWriteMutex);
    for (int i = 0; i < MAX_ACCESSORIES; i++)
        home[i].status = DELETED;
    pthread_cond_broadcast(&updateCond);
    pthread_mutex_unlock(&readWriteMutex);
    pthread_mutex_lock(&deleteMutex);
    for (int i = 0; i < homeIndex; i++)
        sem_wait(deleteSem);
    pthread_mutex_unlock(&deleteMutex);
    puts("\n<SERVER> Eliminati tutti gli accessori");

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
        pthread_cancel(threadPool[i]);
    puts("<SERVER> Eliminati tutti i thread della pool");

    check(shmdt((void *) portSHM), "shmdt");
    check(shmctl(shmID, IPC_RMID, 0), "shmctl");
    puts("<SERVER> Eliminata shared memory");

    check(msgctl(msgID, IPC_RMID, NULL), "msgctl");
    puts("<SERVER> Eliminata coda di messaggi");

    check(sem_close(deleteSem), "sem_close");
    check(sem_unlink("progSisOpD"), "sem_unlink");
    check(sem_close(emptyQueueSem), "sem_close");
    check(sem_unlink("progSisOpE"), "sem_unlink");
    check(sem_close(fullQueueSem), "sem_close");
    check(sem_unlink("progSisOpF"), "sem_unlink");
    puts("<SERVER> Eliminati semafori POSIX");

    pthread_mutex_destroy(&deleteMutex);
    pthread_mutex_destroy(&readMutex);
    pthread_mutex_destroy(&readWriteMutex);
    pthread_cond_destroy(&updateCond);
    puts("<SERVER> Distrutti mutex e cond");

    close(socketFD);
    close(newSocketFD);
    puts("<SERVER> Socket chiuse");

    puts("\n# Fine del programma (hub)\n");
    exit(EXIT_SUCCESS);
}

void initHome() {
    pthread_mutex_lock(&readWriteMutex);
    for (int i = 0; i < MAX_ACCESSORIES; i++) {
        strcpy(home[i].name, "");
        home[i].status = -1;
    }
    homeIndex = 0;
    pthread_mutex_unlock(&readWriteMutex);
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

void signalHandler(int sig) {
    pthread_cancel(acceptConnectionTID);
}

void * acceptConnection(void * arg) {
    while (true) {
        clientLen = sizeof(clientAddr);
        check(newSocketFD = accept(socketFD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen), "accept");
        printf("<SERVER> Connessione accettata - Porta locale: %d - Porta client: %d\n", *portSHM, ntohs(clientAddr.sin_port));

        Message clientSocket;
        clientSocket.type = MSG_TYPE;
        clientSocket.socket = newSocketFD;
        sem_wait(emptyQueueSem);
        pthread_mutex_lock(&queueMutex);
        check(msgsnd(msgID, &clientSocket, sizeof(Message), 0), "msgsnd");
        pthread_mutex_unlock(&queueMutex);
        sem_post(fullQueueSem);
    }
    pthread_exit(EXIT_SUCCESS);
}

void * threadFunction(void * arg) {
    while (true) {
        Message clientSocket;
        sem_wait(fullQueueSem);
        pthread_mutex_lock(&queueMutex);
        check(msgrcv(msgID, &clientSocket, sizeof(Message), MSG_TYPE, 0), "msgrcv");
        pthread_mutex_unlock(&queueMutex);
        sem_post(emptyQueueSem);
        requestHandler(clientSocket.socket);
    }
    pthread_exit(EXIT_SUCCESS);
}

void requestHandler(int newSocketFD) {
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
                    printf("\t\t<Thread> %s impostato a %d\n", home[i].name, home[i].status);
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
            pthread_mutex_lock(&deleteMutex);
            for (int i = 0; i < homeIndex; i++)
                sem_wait(deleteSem);
            pthread_mutex_unlock(&deleteMutex);
            initHome();
            break;
        case 7:
            // Add accessory to hub
            pthread_mutex_lock(&readWriteMutex);
            myIndex = homeIndex++;
            strcpy(home[myIndex].name, packet.accessory.name);
            home[myIndex].status = 0;
            printf("\t\t<Thread> Aggiunto %s\n", home[myIndex].name);
            strcpy(tempInfo.name, home[myIndex].name);
            tempInfo.status = home[myIndex].status;
            // Keeping accessories updated
            while (true) {
                pthread_cond_wait(&updateCond, &readWriteMutex);
                if (tempInfo.status != home[myIndex].status) {
                    send(newSocketFD, &home[myIndex], sizeof(home[myIndex]), 0);
                    tempInfo.status = home[myIndex].status;
                    printf("\t\t<Thread> Inviato aggiornamento (%d) all'accessorio %s\n", tempInfo.status, tempInfo.name);
                    if (home[myIndex].status == DELETED) {
                        printf("\t\t<Thread> %s eliminato\n", tempInfo.name);
                        if (close(newSocketFD) == 0)
                            printf("<Thread> Connessione accessorio terminata\n");
                        pthread_mutex_unlock(&readWriteMutex);
                        sem_post(deleteSem);
                        return;
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
}

void startReading() {
    pthread_mutex_lock(&readMutex);
    readCount++;
    if (readCount == 1)
        pthread_mutex_lock(&readWriteMutex);
    pthread_mutex_unlock(&readMutex);
}

void endReading() {
    pthread_mutex_lock(&readMutex);
    readCount--;
    if (readCount == 0)
        pthread_mutex_unlock(&readWriteMutex);
    pthread_mutex_unlock(&readMutex);
}

void addrInitServer(struct sockaddr_in *address, long IPaddr, int port) {
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(IPaddr);
    address->sin_port = htons(port);
}