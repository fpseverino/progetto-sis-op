#include "libraries.h"

#define PORT 12345
#define BUFF_SIZE 4096

void addrInit(struct sockaddr_in *address, int port);

int main() {
    int socketD, result;
    char buff[BUFF_SIZE];
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);

    puts("\n# Inizio del programma\n");
    puts("<CLIENT> in esecuzione...");
    addrInit(&serverAddr, PORT);

    socketD = socket(PF_INET, SOCK_STREAM, 0);
    if (socketD == -1) {
        perror("Errore creazione socket");
        exit(EXIT_FAILURE);
    }

    result = connect(socketD, (struct sockaddr_in *) &serverAddr, sizeof(serverAddr));
    if (result == -1) {
        perror("Errore connect");
        exit(EXIT_FAILURE);
    }
    getsockname(socketD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);
    printf("<CLIENT> Connessione stabilita\n\tPorta server: %d\n\tPorta locale: %d\n", PORT, ntohs(clientAddr.sin_port));

    send(socketD, "Hello, world!", sizeof("Hello, world!"), 0);
    recv(socketD, buff, sizeof(buff), 0);
    printf("<CLIENT> Risposta del server: %s\n", buff);

    close(socketD);
    puts("\n# Fine del programma\n");
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, int port) {
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    inet_aton("127.0.0.1", &address->sin_addr);
}