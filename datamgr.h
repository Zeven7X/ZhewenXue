/**
 * \author Zhewen Xue
 */

#ifndef _DATAMGR_H_
#define _DATAMGR_H_

#include <stdio.h>
#include <stdlib.h>
#include "lib/dplist.h"
#include "sbuffer.h"
#include <inttypes.h>
#include "datamgr.h"
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "config.h"

#ifndef RUN_AVG_LENGTH
#define RUN_AVG_LENGTH 5
#endif

#ifndef SET_MAX_TEMP
#error SET_MAX_TEMP not set
#endif

#ifndef SET_MIN_TEMP
#error SET_MIN_TEMP not set
#endif
typedef struct{
    sensor_id_t sensor_id;         /** < sensor id */
    sensor_value_t value;   /** < sensor average value */
    sensor_ts_t last_modified;         /** < sensor timestamp */
    room_id_t room_id;   /**<room id*/
    sensor_value_t data[RUN_AVG_LENGTH];
}sensor_node_t;

struct dplist_node{
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;

    void *(*element_copy)(void *src_element);

    void (*element_free)(void **element);

    int (*element_compare)(void *x, void *y);
};
/*
 * Use ERROR_HANDLER() for handling memory allocation problems, invalid sensor IDs, non-existing files, etc.
 */
#define ERROR_HANDLER(condition, ...)    do {                       \
                      if (condition) {                              \
                        printf("\nError: in %s - function %s at line %d: %d\n", __FILE__, __func__, __LINE__, __VA_ARGS__); \
                        exit(EXIT_FAILURE);                         \
                      }                                             \
                    } while(0)

void datamgr_read_room_file(FILE *fp_sensor_map);
/**
 *  This method holds the core functionality of your datamgr. It takes in 2 file pointers to the sensor files and parses them. 
 *  When the method finishes all data should be in the internal pointer list and all log messages should be printed to stderr.
 *  \param fp_sensor_map file pointer to the map file
 *  \param fp_sensor_data file pointer to the binary data file
 */
void datamgr_parse_sensor_files( sbuffer_t* sbuffer, pthread_barrier_t* barrier);

/**
 * This method should be called to clean up the datamgr, and to free all used memory. 
 * After this, any call to datamgr_get_room_id, datamgr_get_avg, datamgr_get_last_modified or datamgr_get_total_sensors will not return a valid result
 */
void datamgr_free();

/**
 * Gets the room ID for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the corresponding room id
 */
uint16_t datamgr_get_room_id(sensor_id_t sensor_id);

/**
 * Gets the running AVG of a certain senor ID (if less then RUN_AVG_LENGTH measurements are recorded the avg is 0)
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the running AVG of the given sensor
 */
sensor_value_t datamgr_get_avg(sensor_id_t sensor_id);

/**
 * Returns the time of the last reading for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the last modified timestamp for the given sensor
 */
time_t datamgr_get_last_modified(sensor_id_t sensor_id);

/**
 *  Return the total amount of unique sensor ID's recorded by the datamgr
 *  \return the total amount of sensors
 */
int datamgr_get_total_sensors();

// define callback functions
void element_free(void** element);
int element_compare(void *x, void* y);
void* element_copy(void* element);
//define selfmade functions
sensor_node_t * datamgr_match_element(sensor_id_t sensorId,dplist_t* sensor_list,int *err);//get the sensor node by using id
#endif  //_DATAMGR_H_
