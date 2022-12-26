//
//  libraries.h
//  progettoSisOp
//
//  Created by Francesco Paolo Severino and Roberto Giovanni Scolari on 13/12/22.
//

#ifndef LIBRARIES_H
#define LIBRARIES_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 27000
#define MAX_CONN 8
#define MAX_ACCESSORIES 5
#define EXIT_MENU 8
#define BUFF_SIZE 128
#define DELETED -127

typedef struct {
    char name[128];
    int status;
} Accessory;

typedef struct {
    int request;
    Accessory accessory;
} Packet;

/*
    Packet.request
    1 = Ask permission to add accessory (Add accessory) (device)
    2 = Read status of one accessory
    3 = Read status of all accessories
    4 = Update status of one accessory
    5 = Delete all accessories
    6 = 
    7 = Add accessory to hub (accessory)
    8 = Exit
*/

int deallocateSem(int semID);
int initSem(int semID);
int waitSem(int semID);
int signalSem(int semID);

#endif