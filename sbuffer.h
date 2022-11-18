/**
 * \author Zhewen Xue
 */

#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#include "config.h"
#define SBUFFER_DATAMGR_ALREADY 1
#define SBUFFER_DATAMGR_NOT_YET 0

#define SBUFFER_STRMGR_ALREADY 1
#define SBUFFER_STRMGR_NOT_YET 0

#define SBUFFER_FAILURE_OR_NO_DATA -1
#define SBUFFER_SUCCESS 0
#define SBUFFER_STR_SUCCESS 1
#define SBUFFER_DATA_SUCCESS 2
#define SBUFFER_DATANON_READ 3
#define SBUFFER_STRSUC_DATAFAL 4
#define SBUFFER_DATASUC_STRFAL 5
#define SBUFFER_DELETE_ONE 6

typedef struct sbuffer sbuffer_t;

/**
 * Allocates and initializes a new shared buffer
 * \param buffer a double pointer to the buffer that needs to be initialized
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_init(sbuffer_t **buffer);

/**
 * All allocated resources are freed and cleaned up
 * \param buffer a double pointer to the buffer that needs to be freed
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_free(sbuffer_t **buffer);

/**
 * Removes the first sensor data in 'buffer' (at the 'head') and returns this sensor data as '*data'
 * If 'buffer' is empty, the function doesn't block until new sensor data becomes available but returns SBUFFER_NO_DATA
 * \param buffer a pointer to the buffer that is used
 * \param data a pointer to pre-allocated sensor_data_t space, the data will be copied into this structure. No new memory is allocated for 'data' in this function.
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_remove(sbuffer_t *buffer);

/**
 * Inserts the sensor data in 'data' at the end of 'buffer' (at the 'tail')
 * \param buffer a pointer to the buffer that is used
 * \param data a pointer to sensor_data_t data, that will be copied into the buffer
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
*/
int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data);
int sbuffer_datamgr(sbuffer_t *buffer, sensor_data_t *data);
int sbuffer_strmgr(sbuffer_t *buffer, sensor_data_t *data);
int sbuffer_display(sbuffer_t *buffer);
int sbuffer_size(sbuffer_t *buffer);
void sbuffer_set_through_barrier(sbuffer_t *buffer);
int sbuffer_get_through_barrier(sbuffer_t* buffer);
void write_to_logfile(char *buffer);
void connmgr_write_close_logfile();
#endif  //_SBUFFER_H_
