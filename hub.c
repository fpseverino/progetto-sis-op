#include "libraries.h"

#define SERVER_PORT 12345
#define MAX_CONN 4
#define BUF_SIZE 4096

struct sockaddr_in clientAddr;

void addrInit(struct sockaddr_in *address, long IPaddr, int port);
void * handler(void * clientSocket);

int main() {
    int sd, newSd;
    int result;
    char buffer[BUF_SIZE];
    struct sockaddr_in serverAddr;
    //struct sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);
    pthread_t tid;

    puts("# Inizio del programma\n");
    puts("<SERVER> in esecuzione...");
    addrInit(&serverAddr, INADDR_ANY, SERVER_PORT);


}

void addrInit(struct sockaddr_in *address, long IPaddr, int port) {
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(IPaddr);
    address->sin_port = htons(port);
}