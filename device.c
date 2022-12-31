//
//  device.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

void addrInit(struct sockaddr_in *address, int port);
int mainMenu(int semID);

int main() {
    int socketFD, printSemID;
    struct sockaddr_in serverAddr, clientAddr;
    int clientLen = sizeof(clientAddr);

    Packet packet;
    pid_t pid; // pid of the exec process
    char buff[BUFF_SIZE];
    Accessory tempInfo;
    int accessoryStatus = -1;
    bool OKtoConnect;

    puts("\n# Inizio del programma (device)\n");

    check(printSemID = semget(ftok(".", 'x'), 0, 0), "semget device");
    printf("<CLIENT> Ottenuto semaforo con ID: %d\n", printSemID);

    addrInit(&serverAddr, PORT);
    check(socketFD = socket(PF_INET, SOCK_STREAM, 0), "socket");
    check(connect(socketFD, (struct sockaddr *) &serverAddr, sizeof(serverAddr)), "connect");
    getsockname(socketFD, (struct sockaddr *) &clientAddr, (socklen_t *) &clientLen);
    printf("<CLIENT> Connessione stabilita - Porta server: %d - Porta locale: %d\n", PORT, ntohs(clientAddr.sin_port));

    
    while ((packet.request = mainMenu(printSemID)) != EXIT_MENU) {
        switch (packet.request) {
        case 1:
            // Add accessory
            waitSem(printSemID);
            printf("\nNome dell'accessorio: ");
            fflush(stdin);
            scanf("%s", buff);
            strcpy(packet.accessory.name, buff);
            send(socketFD, &packet, sizeof(Packet), 0);
            recv(socketFD, &OKtoConnect, sizeof(bool), 0);
            if (!OKtoConnect) {
                puts("<CLIENT> Aggiunta non andata a buon fine");
                signalSem(printSemID);
                break;
            }
            signalSem(printSemID);
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
            waitSem(printSemID);
            printf("\nNome dell'accessorio: ");
            fflush(stdin);
            scanf("%s", buff);
            strcpy(packet.accessory.name, buff);
            send(socketFD, &packet, sizeof(Packet), 0);
            recv(socketFD, &accessoryStatus, sizeof(accessoryStatus), 0);
            printf("<CLIENT> Risposta del server: %d\n", (int) accessoryStatus);
            signalSem(printSemID);
            break;
        case 3:
            // Read status of all accessories
            send(socketFD, &packet, sizeof(Packet), 0);
            waitSem(printSemID);
            puts("");
            for (int i = 0; i < MAX_ACCESSORIES; i++) {
                recv(socketFD, &tempInfo.name, sizeof(tempInfo.name), 0);
                recv(socketFD, &tempInfo.status, sizeof(tempInfo.status), 0);
                printf("%s: %d\n", tempInfo.name, tempInfo.status);
            }
            signalSem(printSemID);
            break;
        case 4:
            // Update status of one accessory
            waitSem(printSemID);
            printf("\nNome dell'accessorio: ");
            fflush(stdin);
            scanf("%s", buff);
            strcpy(packet.accessory.name, buff);
            printf("Stato dell'accessorio: ");
            fflush(stdin);
            scanf("%d", &packet.accessory.status);
            send(socketFD, &packet, sizeof(Packet), 0);
            puts("<CLIENT> Update inviato all'hub");
            signalSem(printSemID);
            break;
        case 5:
            // Delete all accessories
            send(socketFD, &packet, sizeof(Packet), 0);
            break;
        default:
            puts("\n<CLIENT> Richiesta non definita");
            break;
        }
        sleep(1);
    }

    send(socketFD, &packet, sizeof(Packet), 0);

    close(socketFD);
    puts("\n# Fine del programma (device)\n");
    exit(EXIT_SUCCESS);
}

void addrInit(struct sockaddr_in *address, int port) {
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    inet_aton("127.0.0.1", &address->sin_addr);
}

int mainMenu(int semID) {
    waitSem(semID);
    printf("%s", "\n*** MENU PRINCIPALE ***\n"
        " 1 - Aggiungi accessorio\n"
        " 2 - Visualizza accessorio\n"
        " 3 - Visualizza elenco accessori\n"
        " 4 - Aggiorna accessorio\n"
        " 5 - Elimina tutti gli accessori\n"
        " 8 - Uscita\n? ");
    int choice;
    fflush(stdin);
    scanf("%d", &choice);
    signalSem(semID);
    return choice;
}