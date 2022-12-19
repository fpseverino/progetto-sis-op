//
//  device.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

#define PORT 12345
#define BUFF_SIZE 128

void addrInit(struct sockaddr_in *address, int port);

int main() {
    int socketFD;
    char buff[BUFF_SIZE];
    int accessoryStatus = -1;
    Packet packet;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);

    puts("\n# Inizio del programma\n");
    puts("<CLIENT> in esecuzione...");
    addrInit(&serverAddr, PORT);

    if ((socketFD = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (connect(socketFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    getsockname(socketFD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);
    printf("<CLIENT> Connessione stabilita - Porta server: %d - Porta locale: %d\n", PORT, ntohs(clientAddr.sin_port));

    printf("Richiesta: ");
    scanf("%d", &packet.request);
    printf("Nome dell'accessorio: ");
    scanf("%s", buff);
    strcpy(packet.accessory.name, buff);
    send(socketFD, &packet, sizeof(Packet), 0);
    recv(socketFD, &accessoryStatus, sizeof(accessoryStatus), 0);
    printf("<CLIENT> Risposta del server: %d\n", (int) accessoryStatus);

    close(socketFD);
    puts("\n# Fine del programma\n");
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, int port) {
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    inet_aton("127.0.0.1", &address->sin_addr);
}