//
// Created by zhewen on 28.11.20.
//
#define _GNU_SOURCE

#include "connmgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include "lib/tcpsock.h"
#include "lib/dplist.h"
#include "config.h"
#include "datamgr.h"
#include "sensor_db.h"
#include "connmgr.h"
#include "sbuffer.h"
void *elements_copy(void * element);  // Duplicate 'element'; If needed allocated new memory for the duplicated element.
void elements_free(void ** element); // If needed, free memory allocated to element
int elements_compare(void * x, void * y);// Compare two element elements; returns -1 if x<y, 0 if x==y, or 1 if x>y
dplist_t * sockets = NULL;

void connmgr_listen(int port_number, sbuffer_t ** buffer){
    FILE * fp_bin ;
    fp_bin = fopen("sensor_data_recv","wb");
    CONNMGR_FILE_ERR_HANDLER(fp_bin);
    //write the data into the sensor_data_recv
    tcpsock_t *server, *new_client, *client;
    socket_connection_t* connection;
    sensor_id_t sensorId=0;
    sensor_ts_t sensorTs=0;
//    time_t times = 0;
    sensor_value_t sensorValue=0;
    struct timeval timeout;
    socket_connection_t new_connections;
    time_t server_ts;
    fd_set fdSet;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    int server_sd,client_sd , max_sd , client_amount, i , select_result, size, result=1, waiting=1;
    sockets = dpl_create(elements_copy,elements_free,elements_compare);
    printf("Socket list is built.\n");
    int resu = tcp_passive_open(&server,port_number);
    if(resu!=TCP_NO_ERROR){
        printf("open server tcp socket failed. result = %d\n",resu);
        exit(EXIT_FAILURE);
    }
    printf("port number is %d.\n",port_number);
    server_sd = socket_get_sd(server);
    server_ts = time(NULL);
    max_sd = server_sd;
    bool terminate = false;
    printf("terminal is not true.\n");
    while(!terminate){
        server_sd = socket_get_sd(server);
        FD_ZERO(&fdSet);
        FD_SET(server_sd,&fdSet);
        max_sd = server_sd;
        client_amount = dpl_size(sockets);
        for(i=0;i<client_amount;i++){
            connection = (socket_connection_t*)dpl_get_element_at_index(sockets,i);
            client = socketGetTcpSockets(connection);
            client_sd = socket_get_sd(client);
            if(client_sd>0) FD_SET(client_sd,&fdSet);
            if(client_sd>max_sd)   max_sd = client_sd;
        }
        select_result = select(max_sd+1,&fdSet,NULL,NULL,&timeout);
        if(select_result<0){
            perror("Select error.\n");
            exit(EXIT_FAILURE);
        }
        //this judgement means that because of the tcp connection, the server socket has also changed the statement
        if(FD_ISSET(server_sd,&fdSet)){
            //there is a new connection
            server_ts = time(NULL);
            if(tcp_wait_for_connection(server,&new_client)!=TCP_NO_ERROR) exit(EXIT_FAILURE);
            //add the new socket into the dplist
            new_connections.socket = new_client;
            new_connections.ts = time(NULL);
            new_connections.wirte_to_log=NOT_WRITTEN;
            new_connections.ID = 0;
            sockets = dpl_insert_at_index(sockets,(void*)&new_connections,client_amount+1,true);
            //the max_sd will be changed at the first for loop, and then be used in the select function.
        }
        client_amount = dpl_size(sockets);

        for(i=0;i<client_amount;i++){
            //these codes my own function
            connection = (socket_connection_t*)dpl_get_element_at_index(sockets,i);
            client = socketGetTcpSockets(connection);
            client_sd = socket_get_sd(client);
            //has already get client_sd
            if(FD_ISSET(client_sd,&fdSet)){
                //which means this tcp has data
//                times = time(NULL);
                if(connection==NULL){
                    printf("read socket error.\n");
                    exit(EXIT_FAILURE);
                }
                size = sizeof(sensor_id_t);
                result= tcp_receive(client,(void*)&sensorId,&size);
                //printf("%d\n",judge1);
                size = sizeof(sensor_value_t);
                result= tcp_receive(client,(void*)&sensorValue,&size);
                //printf("%d\n",judge2);
                size = sizeof(sensor_ts_t);
                result = tcp_receive(client,(void*)&sensorTs,&size);
                //printf("%d\n",judge3);
                if(result==TCP_NO_ERROR){
                    //write the data to the sensor_data_recv;
                    sensor_data_t *data = (sensor_data_t*)malloc(sizeof(sensor_data_t));
                    fwrite((void*)&sensorId, sizeof(sensor_id_t), 1, fp_bin);
                    fwrite((void*)&sensorValue, sizeof(sensor_value_t), 1, fp_bin);
                    fwrite((void*)&sensorTs, sizeof(time_t), 1, fp_bin);
                    printf("SensorId is %d Temp is %f timestamp is %ld\n\n", sensorId, sensorValue, sensorTs);
                    fprintf(fp_bin, "%d %f %ld\n", sensorId, sensorValue, sensorTs);
                    if(connection->wirte_to_log==NOT_WRITTEN){    //write to log that there is one new connection.
                        char * info=(char *)malloc(100*sizeof(char));
                        char idstring[5];
                        sprintf(idstring,"%u",sensorId);
                        strcpy(info," A sensor node with sensor id ");
                        strcat(info, idstring);
                        strcat(info, " has opened a new connection \n");
                        write_to_logfile( info);
                        free(info);
                        connection->ID=sensorId;
                        connection->wirte_to_log=ALREADY_WRITTEN;
                    }if(connection->wirte_to_log==ALREADY_WRITTEN&&sensorTs-connection->ts>TIMEOUT){
                        //check if the connection is timeout
                        printf("sensor id = %d expired time %ld \n",sensorId,sensorTs-connection->ts);
                        char * info=(char *)malloc(100*sizeof(char));
                        char idstring[5];
                        sprintf(idstring,"%u",sensorId);
                        strcpy(info," The sensor node with ");
                        strcat(info, idstring);
                        strcat(info, " has closed the connection \n");
                        write_to_logfile( info);
                        free(info);
                        tcp_close(&client);
                        free(connection);
                        dpl_remove_at_index(sockets,i,0);
                    }else{//the connection is OK, so record these values.
                        connection->ts = sensorTs;
                        data->ts = sensorTs;
                        data->value = sensorValue;
                        data->id = sensorId;
                        if((sbuffer_insert(*buffer,data))!=-1){
                            printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data->id, data->value, data->ts);
                        }else{
                            printf("connmgr write to sbuffer failed.\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                    free(data);
                }
                else {// if the connection closed(runs loop times) print close the connection.
//                    CONNMGR_SELECT_ERROR(result);
                    sensorId = connection->ID;
                    char * info=(char *)malloc(100*sizeof(char));
                    char idstring[5];
                    sprintf(idstring,"%u",sensorId);
                    strcpy(info," The sensor node with ");
                    strcat(info, idstring);
                    strcat(info, " has closed the connection \n");
                    write_to_logfile( info);
                    free(info);
                    dpl_remove_at_index(sockets,i,true);
                    printf("sensor id %d deleted in the socket.ts = %ld\n", sensorId,time(NULL));
                }
                //Determine the problem of receiving data
            }//if(FD_ISSET)
        }//for loop

        //now, to check the connections
//        check_clients(sockets,times);
        //check if there are no connections
        client_amount = dpl_size(sockets);
        if(client_amount>0){
            waiting = 0;
        }else{
            if(client_amount==0&&waiting == 0){
                waiting = 1;
                server_ts = time(NULL);
            }
            if(waiting==1&&time(NULL) - server_ts>TIMEOUT){
                //connection is timeout close the connection
                if(tcp_close(&server)!=TCP_NO_ERROR){
                    printf("The process of closing server failed.\n");
                    exit(EXIT_FAILURE);
                }
                printf("TIMEOUT, close the connection. ts = %ld\n",time(NULL));
                if(fclose(fp_bin)==0){
                    printf("Succeed in closing the FILE pointer!\n");}
                terminate = true;
            }


        }
    }//while loop

}
void connmgr_free(){
    dpl_free(&sockets,true);
    printf("connmgr dplist has been deleted.\n");

}



void * elements_copy(void * element){
    socket_connection_t * new=(socket_connection_t*)malloc(sizeof(socket_connection_t));
    new->socket= socketGetTcpSockets((socket_connection_t*)element);
    new->ts=((socket_connection_t*)element)->ts;
    new->wirte_to_log = ((socket_connection_t*)element)->wirte_to_log;
    new->ID = ((socket_connection_t*)element)->ID;
    return new;
}

void elements_free(void ** element){
    tcp_sock_free (((socket_connection_t*)(*element))->socket);
    free((socket_connection_t* )*element);
}
//nothing compares here
int elements_compare(void *x, void *y){
    socket_connection_t * ASocket = (socket_connection_t*)  x;
    socket_connection_t * BSocket = (socket_connection_t*)  y;
    if(ASocket==NULL||BSocket==NULL){
        printf("element_compare has one empty element.\n");
        return 0;
    }else{
        if(ASocket->ts>=BSocket->ts)
            return 1;//x larger;
        else
            return 0;
    }
}
tcpsock_t* socketGetTcpSockets(socket_connection_t* connect){
    if(connect==NULL){
        printf("This element in the dplist is NULL.\n");
        exit(EXIT_FAILURE);
    }
    return connect->socket;

}
time_t socketGetTimeStamp(socket_connection_t* connect){
    if(connect==NULL){
        printf("This element in the dplist is NULL.\n");
        exit(EXIT_FAILURE);
    }
    return connect->ts;
}

void  check_clients(dplist_t* list, time_t times){
    int size = dpl_size(list);
    socket_connection_t * socket;
    time_t timer = 0;
    sensor_id_t ids ;
    tcpsock_t * tcpsock;
    for(int j = 0; j<size;j++){
        socket = dpl_get_element_at_index(list,j);
        ids = socket->ID;
        timer = socketGetTimeStamp(socket);
        tcpsock = socketGetTcpSockets(socket);
        if(times - timer > TIMEOUT){
            time_t expired = times - timer;
            printf("sensor id = %d expired time %ld \n",ids,expired);
            char * info=(char *)malloc(100*sizeof(char));
            char idstring[5];
            sprintf(idstring,"%u",ids);
            strcpy(info," The sensor node with ");
            strcat(info, idstring);
            strcat(info, " has closed the connection \n");
            write_to_logfile( info);
            free(info);
            tcp_close(&tcpsock);
            free(socket);
            dpl_remove_at_index(list,j,0);
            j--;
        }
        if(dpl_size(list)==0){
            break;
        }
    }
}

