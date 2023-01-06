//
//  libraries.h
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#ifndef LIBRARIES_H_
#define LIBRARIES_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define SERVER_BACKLOG 100 // listen()
#define MAX_ACCESSORIES 5 // lenght of home array
#define EXIT_MENU 6
#define BUFF_SIZE 128
#define DELETED -127 // if an accessory gets this status, it's about to get deleted
#define THREAD_POOL_SIZE 16
#define MSG_TYPE 1
#define MAX_REQUEST 32 // "size" of messages queue (producer-consumer problem)

#if defined(_SEM_SEMUN_UNDEFINED)
union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                (Linux-specific) */
};
#endif

// Messages queue struct
typedef struct {
    long type;
    int socket;
} Message;

// Struct holding accessory info
typedef struct {
    char name[128];
    int status;
} Accessory;

// Packet for sending accessory info with a request number
typedef struct {
    int request;
    Accessory accessory;
} Packet;

/*
    Packet.request
    1 = Ask permission to add accessory (sent by device)
    2 = Read status of one accessory
    3 = Read status of all accessories
    4 = Update status of one accessory
    5 = Delete all accessories
    6 = Exit
    7 = Add accessory to hub (sent by accessory)
*/

void check(int result, char * message);

// System V semaphores
int deallocateSem(int semID);
int initSem(int semID, int initiaValue);
int waitSem(int semID);
int signalSem(int semID);

void addrInitClient(struct sockaddr_in *address, int port);

#endif