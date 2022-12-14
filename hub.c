#include "libraries.h"

#define PORT 12345
#define MAX_CONN 4
#define BUFF_SIZE 4096

struct sockaddr_in clientAddr;

void addrInit(struct sockaddr_in *address, long IPaddr, int port);
void * threadHandler(void * clientSocket);

int main() {
    int socketD, newSocketD;
    int result;
    struct sockaddr_in serverAddr;
    int clientLen = sizeof(clientAddr);
    pthread_t tid;

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
        printf("<SERVER> Connessione accettata\n\tPorta locale: %d\n\tPorta client: %d\n", PORT, ntohs(clientAddr.sin_port));

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
    char buff[BUFF_SIZE];
    printf("<Thread> Gestisco connessione\n\tPorta locale: %d\n\tPorta client: %d\n", PORT, ntohs(clientAddr.sin_port));
    recv(socketD, buff, sizeof(buff), 0);
    printf("<Thread> Dati ricevuti: %s\n", buff);
    strcpy(buff, "OK");
    send(socketD, buff, sizeof(buff), 0);
    if (close(socketD) == 0)
        printf("<Thread> Connessione terminata\n\tPorta locale: %d\n\tPorta client: %d\n", PORT, ntohs(clientAddr.sin_port));
    pthread_exit(EXIT_SUCCESS);
}