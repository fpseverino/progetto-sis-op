//
//  libraries.c
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#include "libraries.h"

Node * head = NULL;
Node * tail = NULL;

void enqueue(int * clientSocket) {
    Node * newNode = malloc(sizeof(Node));
    newNode->clientSocket = clientSocket;
    newNode->next = NULL;
    if (tail == NULL) {
        head = newNode;
    } else {
        tail->next = newNode;
    }
    tail = newNode;
}

// returns NULL if the queue is empty.
// Returns the pointer to a clientSocket, if there is one to get
int * dequeue() {
    if (head == NULL) {
        return NULL;
    } else {
        int * result = head->clientSocket;
        Node * temp = head;
        head = head->next;
        if (head == NULL) {tail = NULL;}
        free (temp) ;
        return result;
    }
}

int deallocateSem(int semID) {
    union semun useless;
    return semctl(semID, 0, IPC_RMID, useless);
}

int initSem(int semID, int initiaValue) {
    union semun argument;
    unsigned short values[1];
    values[0] = initiaValue; // initial value
    argument.array = values;
    return semctl(semID, 0, SETALL, argument);
}

int waitSem(int semID) {
    struct sembuf operations[1];
    operations[0].sem_num = 0; // using the first and only semaphore
    operations[0].sem_op = -1; // decrements semaphore value
    operations[0].sem_flg = SEM_UNDO; // automatic UNDO at exit
    return semop(semID, operations, 1); 
}

int signalSem(int semID) {
    struct sembuf operations[1];
    operations[0].sem_num = 0; // using the first and only semaphore
    operations[0].sem_op = 1; // increments semaphore value
    operations[0].sem_flg = SEM_UNDO; // automatic UNDO at exit
    return semop(semID, operations, 1); 
}

void check(int result, char * message) {
    if (result < 0) {
        perror(message);
        exit(EXIT_FAILURE);
    }
}