//
// Created by zhewen on 28.11.20.
//

#ifndef _CONNMGR_H
#define _CONNMGR_H


#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <assert.h>
#include <inttypes.h>
#include <time.h>
#include "lib/tcpsock.h"
#include "lib/dplist.h"
#include "config.h"
#include "sbuffer.h"

#define NOT_WRITTEN 0
#define ALREADY_WRITTEN 1
#ifndef TIMEOUT
#error TIMEOUT not set
#endif

typedef struct socket_connection{
    tcpsock_t* socket;
    time_t	  ts;
    int wirte_to_log;
    sensor_id_t ID;

} socket_connection_t;

//This method holds the core functionality of your connmgr.
// It starts listening on the given port and when when a sensor node connects it writes the data to a sensor_data_recv file.
// This file must have the same format as the sensor_data file in assignment 6 and 7.
void connmgr_listen(int port_number, sbuffer_t ** buffer);

//This method should be called to clean up the connmgr, and to free all used memory.
// After this no new connections will be accepted
void connmgr_free();

tcpsock_t* socketGetTcpSockets(socket_connection_t* connect);
time_t socketGetComeTimeStamp(socket_connection_t* connect);
time_t socketGetTimeStamp(socket_connection_t* connect);
void check_clients(dplist_t* list,time_t timer);
#endif //_CONNMGR_H