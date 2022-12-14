#include "libraries.h"

#define PORT 12345
#define MAX_CONN 4

struct sockaddr_in clientAddr;

void addrInit(struct sockaddr_in *address, long IPaddr, int port);
void * threadHandler(void * clientSocket);

Accessory home[5];

int main() {
    int socketD, newSocketD;
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

    socketD = socket(PF_INET, SOCK_STREAM, 0);
    if (socketD == -1) {
        perror("Errore creazione socket");
        exit(EXIT_FAILURE);
    }

    result = bind(socketD, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (result == -1) {
        perror("Errore bind");
        exit(EXIT_FAILURE);
    }

    if (listen(socketD, MAX_CONN) < 0) {
        perror("Errore listen");
        exit(EXIT_FAILURE);
    }

    printf("<SERVER> in attesa di connessione sulla porta: %d\n", ntohs(serverAddr.sin_port));

    while (true) {
        newSocketD = accept(socketD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);
        if (newSocketD == -1) {
            perror("Errore accept");
            exit(EXIT_FAILURE);
        }
        printf("<SERVER> Connessione accettata - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));

        result = pthread_create(&tid, NULL, threadHandler, (void *) &newSocketD);
        if (result != 0) {
            perror("Errore pthread_create");
            exit(EXIT_FAILURE);
        }
        puts("<SERVER> Thread generato");
    }

    close(socketD);
    close(newSocketD);
    puts("\n# Fine del programma\n");
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, long IPaddr, int port) {
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(IPaddr);
    address->sin_port = htons(port);
}

void * threadHandler(void * clientSocket) {
    int socketD = * (int *) clientSocket;
    Accessory tempAccessory;
    printf("<Thread> Gestisco connessione - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));
    recv(socketD, tempAccessory.name, sizeof(tempAccessory.name), 0);
    printf("<Thread> Dati ricevuti: %s\n", tempAccessory.name);
    for (int i = 0; i < 5; i++) {
        if (strcmp(tempAccessory.name, home[i].name) == 0)
            send(socketD, &home[i].status, sizeof(home[i].status), 0);
    }
    
    if (close(socketD) == 0)
        printf("<Thread> Connessione terminata - Porta locale: %d - Porta client: %d\n", PORT, ntohs(clientAddr.sin_port));
    pthread_exit(EXIT_SUCCESS);
}