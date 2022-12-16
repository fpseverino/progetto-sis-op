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
unsigned int mainMenu();

int main() {
    int socketD, result;
    char buff[BUFF_SIZE];
    int accessoryStatus = -1;
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

    result = connect(socketD, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (result == -1) {
        perror("Errore connect");
        exit(EXIT_FAILURE);
    }
    getsockname(socketD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);
    printf("<CLIENT> Connessione stabilita - Porta server: %d - Porta locale: %d\n", PORT, ntohs(clientAddr.sin_port));

            printf("Nome dell'accessorio: ");
            scanf("%s", buff);
            send(socketD, buff, sizeof(buff), 0);
            recv(socketD, &accessoryStatus, sizeof(accessoryStatus), 0);
            printf("<CLIENT> Risposta del server: %d\n", (int) accessoryStatus);

    close(socketD);
    puts("\n# Fine del programma\n");
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, int port) {
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    inet_aton("127.0.0.1", &address->sin_addr);
}

unsigned int mainMenu() {
    printf("%s", "*** Main menu ***\n"
        " 1 - Add accessory\n"
        " 2 - Check accessory\n"
        " 3 - Update accessory\n"
        " 4 - Exit\n? ");
    unsigned int menu;
    fflush(stdin);
    scanf("%u", &menu);
    return menu;
}