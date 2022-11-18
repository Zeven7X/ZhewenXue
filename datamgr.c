#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "lib/dplist.h"
#include "sbuffer.h"
#include <inttypes.h>
#include "datamgr.h"
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#ifndef RUN_AVG_LENGTH
  #define RUN_AVG_LENGTH 5
#endif

#ifdef DEBUG
  #define DEBUG_PRINTF(...)                   \
do {                      \
	printf("\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__); \
	printf(__VA_ARGS__);                \
} while(0)
#else
  #define DEBUG_PRINTF(...) (void)0
#endif

#ifndef SET_MAX_TEMP
#error SET_MAX_TEMP not set
#endif

#ifndef SET_MIN_TEMP
#error SET_MIN_TEMP not set
#endif







dplist_t *list = NULL;

void modifyData(sensor_node_t * match, sensor_value_t *lastValue);//update the data array of that specific sensor
sensor_value_t getAverage(sensor_node_t * match);//get the average of that specific sensor
int checkSensorID(sensor_id_t sensorId, dplist_t * dplist);
void datamgr_read_room_file(FILE *fp_sensor_map){
    list = dpl_create(element_copy,element_free,element_compare);

    /*begin to parse map file*/
    room_id_t roomId;
    sensor_id_t sensorId;
    sensor_node_t node;
    while(fscanf(fp_sensor_map,"%16hu %16hu\n" ,&roomId,&sensorId)!=EOF)
    {
//        index=dpl_get_index_of_element(list,(void*)&sensorId);
        printf("%16hu %16hu\n",roomId,sensorId);
        if(checkSensorID(sensorId,list)){
            continue;//this means the sensor ID has exited. It is invalid.
        }
        node.room_id=roomId;
        node.sensor_id=sensorId;
        node.value = 0;
        node.last_modified=time(NULL);
        for(int i=0;i<RUN_AVG_LENGTH;i++)
        {
            node.data[i]=0.0;
        }
        dpl_insert_at_index(list,&node,0,true);
    }
    printf("The dplist of map file has been created.\n");
}

void datamgr_parse_sensor_files( sbuffer_t* sbuffer, pthread_barrier_t* barrier){
//    if(sbuffer_get_through_barrier(sbuffer)==0||sbuffer_get_through_barrier(sbuffer)==-1){
//        return ;
//    }

    int result;
    int res;
    /*begin to parse data in sbuffer*/

    sensor_value_t* last_value = malloc(sizeof(sensor_value_t));
    sensor_id_t sid =0;
    sensor_data_t *sensorNode= malloc(sizeof (sensor_data_t));
    sensor_node_t *match ;
    int *err = malloc(sizeof (int));
    while(sbuffer_size(sbuffer)>0){
//        printf("prepare to do sbuffer_datamgr. From datamgr.c.104, time = %d\n",(int)time(NULL));
        result = sbuffer_datamgr(sbuffer,sensorNode);
//        if(result == SBUFFER_DATA_SUCCESS){
        if(result == SBUFFER_DATA_SUCCESS||result == SBUFFER_DATASUC_STRFAL){
            sid =  sensorNode->id;
            printf("sid = %d\n",sid);
            match = datamgr_match_element(sid,list,err);
            if(match==NULL){
                printf("This sensor id does not exist, err info %d\n",*err);
                char* info = malloc(100*sizeof(char));
                char id[5];
                sprintf(id,"%d",sid);
                strcpy(info," Received sensor data with invalid sensor node ID ");
                strcat(info,id);
                strcat(info,".\n");
                write_to_logfile(info);
                free(info);
                continue;
            }
            *last_value = sensorNode->value;
            match->last_modified = sensorNode->ts;//update the last time
            modifyData(match,last_value);//update the data[]
            match->value = getAverage(match);//calculate the average;
            match->sensor_id = sid;
        }
        if(result == SBUFFER_DELETE_ONE||result == SBUFFER_DATA_SUCCESS){
            printf("pthread_barrier_t block in datamgr.c\n");
            res=pthread_barrier_wait(barrier);
            if(res == PTHREAD_BARRIER_SERIAL_THREAD){
                sbuffer_remove(sbuffer);
                printf("Delete one sbuffer. From datamgr.c\n");
            }
            else{
                printf("This is not the first one waiting the barrier. From datamgr.c \n");
                usleep(500000);
            }
//            sbuffer_set_through_barrier(sbuffer);
        }
    }
    free(last_value);
    free(sensorNode);
    free(err);
}

//get the sensor node by using id
sensor_node_t * datamgr_match_element(sensor_id_t sensorId, dplist_t* sensor_list,int *err){
	if(sensor_list == NULL)	{
	    *err = 1;
        return NULL;

	}
	dplist_node_t * node = sensor_list->head;
	if(node == NULL){
	    *err = 2;
	    return NULL;
	}
	sensor_node_t * match = node->element;
//    printf("sensor node ID = %d\n",match->sensor_id);
	while(node->next!=NULL){
		if(match->sensor_id == sensorId)	{
		    *err = 0;
		    return match;}
		else{
			node = node->next;
			match = node->element;}
//        printf("sensor node ID = %d\n",match->sensor_id);
	}
	if(match->sensor_id == sensorId)	{
	    *err = 0;
	    return match;
	}
	else	{
	    *err = 3;
	    return NULL;
	}
}

//update the data array of that specific sensor
void modifyData(sensor_node_t * match, sensor_value_t*lastValue){
	int size = RUN_AVG_LENGTH;
	int judgement = 0;//to check if the function could end
	//first to check if the array is full or not
	for(int i=0;i<size;i++){
		if(match->data[i]==0.0){
			match->data[i] = *lastValue;
			judgement = 1;
			break;
		}
		//this means that end of the array has the latest value
	}if(judgement != 1){
	//second to move all the data forward 1 unit
		for(int j = 1;j<size;j++){
			match->data[j-1]=match->data[j];
		}
		match->data[size-1]=*lastValue;
	}
}
sensor_value_t getAverage(sensor_node_t * match){//get the average of that specific sensor
	double total = 0.0;
	int count = 0;
	int size = RUN_AVG_LENGTH;
	for(int i =0;i<size;i++){
		if(match->data[i]!=0.0)	count++;
		total+=match->data[i];
	}double avg = total/count;

	sensor_id_t sid = match->sensor_id;
    char id[5];
    sprintf(id,"%d",sid);
    char average[10];
    sprintf(average,"%f",avg);
	if(avg>=SET_MAX_TEMP){
        char * info=(char *)malloc(100*sizeof(char));
        strcpy(info," The sensor node with ");
        strcat(info, id);
        strcat(info, " reports it’s too hot (running avg temperature = ");
        strcat(info,average);
        strcat(info,")\n");
        write_to_logfile( info);
        free(info);
	}else if(avg<=SET_MIN_TEMP){
        char * info=(char *)malloc(100*sizeof(char));
        strcpy(info," The sensor node with ");
        strcat(info, id);
        strcat(info, " reports it’s too cold (running avg temperature = ");
        strcat(info,average);
        strcat(info,")\n");
        write_to_logfile( info);
        free(info);
	}
	return avg;
}
/**
 * This method should be called to clean up the datamgr, and to free all used memory. 
 * After this, any call to datamgr_get_room_id, datamgr_get_avg, datamgr_get_last_modified or datamgr_get_total_sensors will not return a valid result
 */
void datamgr_free(){
	dpl_free(&list,true);
	list = NULL;
}

/**
 * Gets the room ID for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the corresponding room id
 */
uint16_t datamgr_get_room_id(sensor_id_t sensor_id){
	sensor_id_t * sid = &sensor_id;
	int *err = malloc(sizeof(int));
	sensor_node_t* match = datamgr_match_element(*sid,list,err);
	free(err);
	
	return match->room_id;
}

/**
 * Gets the running AVG of a certain senor ID (if less then RUN_AVG_LENGTH measurements are recorded the avg is 0)
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the running AVG of the given sensor
 */
sensor_value_t datamgr_get_avg(sensor_id_t sensor_id){
	sensor_id_t * sid = &sensor_id;
    int *err = malloc(sizeof(int));
    sensor_node_t* match = datamgr_match_element(*sid,list,err);
    free(err);
        
	return match->value;

}

/**
 * Returns the time of the last reading for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the last modified timestamp for the given sensor
 */
time_t datamgr_get_last_modified(sensor_id_t sensor_id){
	sensor_id_t * sid = &sensor_id;
    int *err = malloc(sizeof(int));
    sensor_node_t* match = datamgr_match_element(*sid,list,err);
    free(err);
        
	return match->last_modified;
}

/**
 *  Return the total amount of unique sensor ID's recorded by the datamgr
 *  \return the total amount of sensors
 */
int datamgr_get_total_sensors(){
	return dpl_size(list);
}
//callback functions
void * element_copy(void * element) {
    sensor_node_t* copy = malloc(sizeof (sensor_node_t));
    //vsprintf(&new_id,"%I16u",((sensor_node_t*)element)->sensor_id);
    assert(copy != NULL);
    copy->sensor_id = ((sensor_node_t*)element)->sensor_id;
    copy->value = ((sensor_node_t*)element)->value;
    copy->last_modified = ((sensor_node_t*)element)->last_modified;
    copy->room_id = ((sensor_node_t*)element)->room_id;
    for(int i = 0; i<RUN_AVG_LENGTH ; i++){
        copy->data[i] = ((sensor_node_t*)element)->data[i];
    }
    return (void *) copy;
}

void element_free(void ** element) {
    free(*element);
    *element = NULL;
}

int element_compare(void * x, void * y) {
    return ((((sensor_node_t*)x)->value < ((sensor_node_t*)y)->value) ? -1 : (((sensor_node_t*)x)->value == ((sensor_node_t*)y)->value) ? 0 : 1);
}

int checkSensorID(sensor_id_t sensorId,dplist_t *list){
    sensor_node_t *node1;
    sensor_id_t sensorId1;
    for(int i=0;i<dpl_size(list);i++){
        node1 = (sensor_node_t*)dpl_get_element_at_index(list,i);
        sensorId1 = node1->sensor_id;
        if(sensorId1==sensorId){
            return 1;// this means that the sensor id exists.
        }
    }
    return 0;
}

















