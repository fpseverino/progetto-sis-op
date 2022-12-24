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
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 21000
#define MAX_CONN 8
#define MAX_ACCESSORIES 5
#define EXIT 5
#define BUFF_SIZE 128

typedef struct {
    char name[128];
    int status;
} Accessory;

typedef struct {
    int request;
    Accessory accessory;
} Packet;

/*
    request
    1 = Add accessory
    2 = Read status of one accessory
    3 = Read status of all accessories
    4 = Update status of one accessory
    5 = Exit
    6 = 
    7 = 
    8 = 
*/

#endif