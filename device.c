//
//  device.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

#define PORT 12345
#define BUFF_SIZE 128

#define MAX_ACCESSORIES 5

void addrInit(struct sockaddr_in *address, int port);
int mainMenu();

int main() {
    int socketFD;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);
    Packet packet;
    char buff[BUFF_SIZE];
    pid_t pid;
    Accessory tempInfo;
    int accessoryStatus = -1;

    puts("\n# Inizio del programma (client)\n");

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

    switch (packet.request = mainMenu()) {
    case 1:
        // Add accessory
        printf("\nNome dell'accessorio: ");
        fflush(stdin);
        scanf("%s", buff);
        switch (pid = fork()) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;
        case 0:
            execl("accessory", "accessory", buff, NULL);
            exit(EXIT_SUCCESS);
            break;
        default:
            break;
        }
        break;
    case 2:
        // Read status of one accessory
        printf("\nNome dell'accessorio: ");
        fflush(stdin);
        scanf("%s", buff);
        strcpy(packet.accessory.name, buff);
        send(socketFD, &packet, sizeof(Packet), 0);
        recv(socketFD, &accessoryStatus, sizeof(accessoryStatus), 0);
        printf("<CLIENT> Risposta del server: %d\n", (int) accessoryStatus);
        break;
    case 3:
        // Read status of all accessories
        send(socketFD, &packet, sizeof(Packet), 0);
        puts("");
        for (int i = 0; i < MAX_ACCESSORIES; i++) {
            recv(socketFD, &tempInfo.name, sizeof(tempInfo.name), 0);
            recv(socketFD, &tempInfo.status, sizeof(tempInfo.status), 0);
            printf("%s: %d\n", tempInfo.name, tempInfo.status);
        }
        break;
    case 4:
        // Update status of one accessory
        printf("\nNome dell'accessorio: ");
        fflush(stdin);
        scanf("%s", buff);
        strcpy(packet.accessory.name, buff);
        printf("Stato dell'accessorio: ");
        fflush(stdin);
        scanf("%d", &packet.accessory.status);
        send(socketFD, &packet, sizeof(Packet), 0);
        puts("<CLIENT> Update inviato all'hub");
        break;
    default:
        puts("\n<CLIENT> Richiesta non definita");
        break;
    }

    close(socketFD);
    puts("\n# Fine del programma (client)\n");
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, int port) {
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    inet_aton("127.0.0.1", &address->sin_addr);
}

int mainMenu() {
    printf("%s", "\n*** MENU PRINCIPALE ***\n"
        " 1 - Aggiungi accessorio\n"
        " 2 - Visualizza accessorio\n"
        " 3 - Visualizza elenco accessori\n"
        " 4 - Aggiorna accessorio\n? ");
    int choice;
    fflush(stdin);
    scanf("%d", &choice);
    return choice;
}