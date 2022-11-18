/**
 * \author Zhewen Xue
 */
#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "sbuffer.h"
#define FIFO_NAME "logFIFO"
#define thread_num 2
static pthread_mutex_t threads;

int sequence_number = 1;
static FILE* fp_FIFO = NULL;
/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
    struct sbuffer_node *next;  /**< a pointer to the next node*/
    sensor_data_t data;         /**< a structure containing the data */
    int datamgr_complete;         /**< a structure determine if the data is processed, 0 means writen not complete, 1 means writen complete */
    int strmgr_complete;         /**< a structure determine if the element is stored, 0 means writen not complete, 1 means writen complete */
    int through_barrier;
} sbuffer_node_t;

/**
 * a structure to keep track of the buffer
 */
struct sbuffer {
    sbuffer_node_t *head;       /**< a pointer to the first node in the buffer */
    sbuffer_node_t *tail;       /**< a pointer to the last node in the buffer */
};

int sbuffer_init(sbuffer_t **buffer) {
    pthread_mutex_init(&threads,NULL);
    pthread_mutex_lock(&threads);
    *buffer = malloc(sizeof(sbuffer_t));
    fp_FIFO = fopen(FIFO_NAME, "w");
    CHECK_FOPEN(fp_FIFO);
    printf("FIFO ready to write.\n");
    if (*buffer == NULL){
        pthread_mutex_unlock(&threads);
        pthread_mutex_destroy(&threads);
        return SBUFFER_FAILURE_OR_NO_DATA;}
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;
    pthread_mutex_unlock(&threads);
    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer) {
    pthread_mutex_lock(&threads);
    sbuffer_node_t *dummy;
    if ((buffer == NULL) || (*buffer == NULL)) {
        pthread_mutex_unlock(&threads);
        pthread_mutex_destroy(&threads);
        return SBUFFER_FAILURE_OR_NO_DATA;
    }
    while ((*buffer)->head) {
        dummy = (*buffer)->head;
        (*buffer)->head = dummy->next;
        free(dummy);
    }
    free(*buffer);
    *buffer = NULL;
    CHECK_FCLOSE(fclose(fp_FIFO));
    pthread_mutex_unlock(&threads);
    pthread_mutex_destroy(&threads);
    return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer) {
    sbuffer_node_t *dummy;
    if (buffer == NULL) return SBUFFER_FAILURE_OR_NO_DATA;
    if (buffer->head == NULL) return SBUFFER_FAILURE_OR_NO_DATA;

    dummy = buffer->head;
    if (buffer->head == buffer->tail) // buffer has only one node
    {
        buffer->head = buffer->tail = NULL;
    } else  // buffer has many nodes empty
    {
        buffer->head = buffer->head->next;
    }
    free(dummy);
    printf("removed one buffer.\n");
    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
    pthread_mutex_lock(&threads);
    sbuffer_node_t *dummy;
    if (buffer == NULL) {
        pthread_mutex_unlock(&threads);
        printf("insert buffer == NULL");
        return SBUFFER_FAILURE_OR_NO_DATA;}
    dummy = malloc(sizeof(sbuffer_node_t));
    if (dummy == NULL){
        pthread_mutex_unlock(&threads);
        return SBUFFER_FAILURE_OR_NO_DATA;}
    sensor_data_t data1 = *data;
    dummy->data = data1;
    dummy->next = NULL;
    dummy->datamgr_complete = SBUFFER_DATAMGR_NOT_YET;
    dummy->strmgr_complete = SBUFFER_STRMGR_NOT_YET;
    dummy->through_barrier =0;
    if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL
    {
        buffer->head = buffer->tail = dummy;
    } else // buffer not empty
    {
        buffer->tail->next = dummy;
        buffer->tail = buffer->tail->next;
    }
    int i = dummy->data.id;
    printf("sbuffer insert one sensor id = %d,sbuffer insert\n",i);
    pthread_mutex_unlock(&threads);
    return SBUFFER_SUCCESS;
}


int sbuffer_datamgr(sbuffer_t *buffer, sensor_data_t *data) {
    pthread_mutex_lock(&threads);
    sbuffer_node_t *dummy;
    if (buffer == NULL || buffer->head == NULL) {
        pthread_mutex_unlock(&threads);
        return SBUFFER_FAILURE_OR_NO_DATA;               //return -1
    }
    dummy = buffer->head;
    if (dummy->datamgr_complete == SBUFFER_DATAMGR_NOT_YET) {//data = 0
        *data = dummy->data;
        dummy->datamgr_complete = SBUFFER_DATAMGR_ALREADY;
        if (dummy->strmgr_complete == SBUFFER_STRMGR_ALREADY) {//data and str == 1
            // this means the data has been processed by datamgr and sensor_db
            pthread_mutex_unlock(&threads);
            return SBUFFER_DATA_SUCCESS;    // 0,1 return 2
        }            //return 5;
        pthread_mutex_unlock(&threads);
        printf("datamgr have read this.From sbuffer_datamgr\n");
        return SBUFFER_DATASUC_STRFAL;                    //0,0 return 5;data manger runs out.
    } else {
        // this means the data has been processed by datamgr and sensor_db
        if (dummy->strmgr_complete == SBUFFER_STRMGR_NOT_YET) {
            pthread_mutex_unlock(&threads);
            return SBUFFER_DATANON_READ;
        } else {
            printf("removed one buffer.From sbuffer_datamgr 1,1\n");
            pthread_mutex_unlock(&threads);
            return SBUFFER_DELETE_ONE;//return 6;
        }
    }
}


int sbuffer_strmgr(sbuffer_t *buffer, sensor_data_t *data){
    pthread_mutex_lock(&threads);
    sbuffer_node_t *dummy;
    if(buffer==NULL||buffer->head==NULL){
        pthread_mutex_unlock(&threads);
        return SBUFFER_FAILURE_OR_NO_DATA;              //return -1;
    }
    dummy = buffer->head;
    if(dummy->strmgr_complete==SBUFFER_STRMGR_NOT_YET){//str = 0
        *data = dummy->data;
        dummy->strmgr_complete=SBUFFER_STRMGR_ALREADY;
        if(dummy->datamgr_complete==SBUFFER_DATAMGR_ALREADY){//data and str == 1
            pthread_mutex_unlock(&threads);
            printf("removed one buffer.From sbuffer_strmgr 0,1\n");
            return SBUFFER_STR_SUCCESS;  }            //return 1;
        pthread_mutex_unlock(&threads);
        return SBUFFER_STRSUC_DATAFAL;                    //return 4;
    }
    else{                                               //str = 1
        //data = 0, str = 1
        if(dummy->datamgr_complete==SBUFFER_DATAMGR_NOT_YET ){
            pthread_mutex_unlock(&threads);
            return SBUFFER_DATANON_READ;//RETURN 3
        }else{
            pthread_mutex_unlock(&threads);
            return SBUFFER_DELETE_ONE;//return 6;
        }
    }
}

int sbuffer_display(sbuffer_t *buffer){
    pthread_mutex_lock(&threads);
    sbuffer_node_t *dummy;
    if(buffer==NULL){
        pthread_mutex_unlock(&threads);
        printf("sbuffer is NULL.\n");
        return -1;
    }
    if(buffer->head==NULL){
        pthread_mutex_unlock(&threads);
        printf("sbuffer head is NULL.\n");
        return -1;
    }if(buffer->tail==NULL){
        pthread_mutex_unlock(&threads);
        printf("sbuffer tail is NULL.\n");
        return -1;
    }
    dummy = buffer->head;
    sensor_data_t datas = dummy->data;
    if(buffer->head==buffer->tail){
        // this means only one data in this buffer
        sensor_value_t sensorValue = datas.value;
        sensor_id_t sensorId = datas.id;
        sensor_ts_t sensorTs = datas.ts;
        pthread_mutex_unlock(&threads);
        printf("only one node in sbuffer_list.\n");
        printf("sensorId = %d, sensor value = %g, sensor ts = %ld , datamgr_complete = %d, strmgr_complete = %d.\n",sensorId,sensorValue,sensorTs,dummy->datamgr_complete,dummy->strmgr_complete);
        return 0;
    }
    int i = 1;
    while(dummy->next!=NULL){
        printf("this is %d node.\n",i);
        printf("sensorId = %d, sensor value = %g, sensor ts = %ld , datamgr_complete = %d, strmgr_complete = %d.\n",dummy->data.id,dummy->data.value,dummy->data.ts,dummy->datamgr_complete,dummy->strmgr_complete);
        i++;
        dummy = dummy->next;
    }
    datas = dummy->data;
    sensor_value_t sensorValue = datas.value;
    sensor_id_t sensorId = datas.id;
    sensor_ts_t sensorTs = datas.ts;
    pthread_mutex_unlock(&threads);
    printf("This is the final one.\n");
    printf("sensorId = %d, sensor value = %g, sensor ts = %ld , datamgr_complete = %d, strmgr_complete = %d.\n",sensorId,sensorValue,sensorTs,dummy->datamgr_complete,dummy->strmgr_complete);
    return 0;
}

int sbuffer_size(sbuffer_t *buffer) {
    pthread_mutex_lock(&threads);
    sbuffer_node_t *dummy;
    if (buffer == NULL) {
        pthread_mutex_unlock(&threads);
        return -1;
    }
    if (buffer->head == NULL) {
        pthread_mutex_unlock(&threads);
        return 0;
    }
    if (buffer->head == buffer->tail) {
        pthread_mutex_unlock(&threads);
        return 1;
    }
    dummy = buffer->head;
    int j = 1;
    while (dummy != buffer->tail) {
        dummy = dummy->next;
        j++;
    }
    pthread_mutex_unlock(&threads);
    return j;
}

void sbuffer_set_through_barrier(sbuffer_t *buffer){
    sbuffer_node_t *dummy;
    dummy = buffer->head;
    dummy->through_barrier = 1;
}
int sbuffer_get_through_barrier(sbuffer_t* buffer){
    sbuffer_node_t *dummy;
    dummy = buffer->head;
    if(dummy==NULL){
        return 0;//no data
    }
    if(dummy->through_barrier==0)   return 1;//normal
    else    return -1;//it means the thread runs too quick.

}
void write_to_logfile(char *buffer){

    FILE* fp = fopen(FIFO_NAME,"w");
    CHECK_FOPEN(fp);
    if(fp==NULL){
        printf("FIFO_FILE not be initiated.\n");
        return;
    }

    char id[5];
    sprintf(id,"%u",sequence_number);
    time_t timer = time(NULL);
    char times[10];
    sprintf(times,"%ld",timer);
    char *info = (char*)malloc(110*sizeof(char));
    strcpy(info,id);
    strcat(info," ");
    strcat(info,times);
    strcat(info,buffer);
    if(fputs(info,fp)==EOF)
    {
        fprintf(stderr, "Error writing data to fifo\n");
        exit(EXIT_FAILURE);
    }
    free(info);
    sequence_number++;
    if(fflush(fp)==EOF){
        perror("write to FIFO failed.");
        exit(1);
    }
    printf("Info send to log : %s\n", buffer);
    CHECK_FCLOSE(fclose(fp));
}
