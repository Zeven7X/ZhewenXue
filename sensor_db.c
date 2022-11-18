#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "sensor_db.h"
#include "sbuffer.h"
#include <unistd.h>
/*
 * Make a connection to the database server
 * Create (open) a database with name DB_NAME having 1 table named TABLE_NAME
 * If the table existed, clear up the existing data if clear_up_flag is set to 1
 * Return the connection for success, NULL if an error occurs
 */
DBCONN * init_connection(char clear_up_flag){
	sqlite3 *db;
	char *error_msg = 0;

	int rc = sqlite3_open(TO_STRING(DB_NAME),&db);

	if(rc != SQLITE_OK){
		fprintf(stderr, "Cannot open database:%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
        char * info1=(char *)malloc(100*sizeof(char));
        strcpy(info1," Connection to SQL server lost.\n");
        write_to_logfile(info1);
        free(info1);
		return NULL;
	}else{
        char * info1=(char *)malloc(100*sizeof(char));
        strcpy(info1," Connection to SQL server established.\n");
        write_to_logfile(info1);
        free(info1);
	}
	char sql[300];
	if(clear_up_flag == 1) {//create a new table
        strcpy(sql,"DROP TABLE IF EXISTS ");
        strcat(sql,TO_STRING(TABLE_NAME));
        strcat(sql,"; CREATE TABLE ");
        strcat(sql,TO_STRING(TABLE_NAME));
        strcat(sql,"(Id INTEGER PRIMARY KEY AUTOINCREMENT, sensor_id INT, sensor_value DECIMAL(4,2), timestamp TIMESTAMP);" );
        rc = sqlite3_exec(db, sql, 0, 0, &error_msg);
        if(rc!=SQLITE_OK){
            fprintf(stderr,"SQL error: %s\n", error_msg);
            sqlite3_free(error_msg);
            sqlite3_close(db);
            return NULL;
        }else{
            char *info  = (char*)malloc(50*sizeof(char));
            strcpy(info," New table ");
            strcat(info, TO_STRING(TABLE_NAME));
            strcat(info," created.\n");
            write_to_logfile(info);
            free(info);
        }
	}else{//create a new table
        strcpy(sql,"CREATE TABLE IF NOT EXISTS ");
        strcat(sql,TO_STRING(TABLE_NAME));
        strcat(sql,"(Id INTEGER PRIMARY KEY AUTOINCREMENT, sensor_id INT, sensor_value DECIMAL(4,2), timestamp TIMESTAMP);" );
        rc = sqlite3_exec(db, sql, 0, 0, &error_msg);
        if(rc!=SQLITE_OK){
            printf("something fail when inserting.\n");
            return NULL;
        }
    }

	return db;
}

/**
 * Disconnect from the database server
 * \param conn pointer to the current connection
 */
void disconnect(DBCONN *conn){
    char * info=(char *)malloc(100*sizeof(char));
    strcpy(info," Connection to SQL server lost.\n");
    write_to_logfile(info);
    free(info);
    sqlite3_close(conn);

}

/**
 * Write an INSERT query to insert a single sensor measurement
 * \param conn pointer to the current connection
 * \param id the sensor id
 * \param value the measurement value
 * \param ts the measurement timestamp
 * \return zero for success, and non-zero if an error occurs
 */
int insert_sensor(DBCONN *conn, sbuffer_t *buffer,pthread_barrier_t* barrier) {
//    if(sbuffer_get_through_barrier(buffer)==0||sbuffer_get_through_barrier(buffer)==-1){
//        return -1;
//    }

    char sql[200];
    int res = -2;
    if (conn == NULL) {
        return -1;
    }
    int result, getdata = 0;
    sensor_data_t *data = malloc(sizeof(sensor_data_t));
    res = sbuffer_strmgr(buffer, data);
    if (res == SBUFFER_STR_SUCCESS || res == SBUFFER_STRSUC_DATAFAL) {
        snprintf(sql, 100, "INSERT INTO %s (sensor_id, sensor_value, timestamp) VALUES(%hu,%f,%ld)",
                 TO_STRING(TABLE_NAME), data->id, data->value, data->ts);
        printf("INSERT INTO %s VALUES(%hu,%f,%ld)\n", TO_STRING(TABLE_NAME), data->id, data->value, data->ts);

        char *err_msg;
        int rc = sqlite3_exec(conn, sql, 0, 0, &err_msg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            free(data);
            return -1;
        }
        getdata = 1;
    }
    if (res == SBUFFER_DELETE_ONE || res == SBUFFER_STR_SUCCESS) { // this means the strmgr need to wait the datamgr to read this sbuffer.
        printf("pthread_barrier_t block in strmgr.c\n");
        result = pthread_barrier_wait(barrier);
        if (result == PTHREAD_BARRIER_SERIAL_THREAD) {
            sbuffer_remove(buffer);
            printf("Delete one sbuffer. From strmgr.c\n");
        } else {
            printf("This is not the first one waiting the barrier. From strmgr.c \n");
            usleep(50000);
        }

    }
    free(data);
    if(getdata == 1)    return 0;
    else    return 1;
}
/**
 * Write an INSERT query to insert all sensor measurements available in the file 'sensor_data'
 * \param conn pointer to the current connection
 * \param sensor_data a file pointer to binary file containing sensor data
 * \return zero for success, and non-zero if an error occurs
 */
//int insert_sensor_from_file(DBCONN *conn, FILE *sensor_data){
//	uint16_t value;
//	double temper;
//	time_t time;
//	while(fread(&value,sizeof(uint16_t),1,sensor_data)>0){
//		fread(&temper,sizeof(double),1,sensor_data);
//		fread(&time,sizeof(time_t),1,sensor_data);
//		int dummy = insert_sensor(conn,value,temper,time);
//		if(dummy!=0)	return -1;
//	}return 0;
//}
/**
  * Write a SELECT query to select all sensor measurements in the table
  * The callback function is applied to every row in the result
  * \param conn pointer to the current connection
  * \param f function pointer to the callback method that will handle the result set
  * \return zero for success, and non-zero if an error occurs
  */
int find_sensor_all(DBCONN *conn, callback_t f){
	char sql[100];
	snprintf(sql,100,"SELECT * FROM %s",TO_STRING(TABLE_NAME));
	char * err_msg = 0;
	int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
	if(rc!=SQLITE_OK){
		fprintf(stderr,"Failed to find all sensor\n");
		sqlite3_free(err_msg);
		return -1;
	}return 0;
}

/**
 * Write a SELECT query to return all sensor measurements having a temperature of 'value'
 * The callback function is applied to every row in the result
 * \param conn pointer to the current connection
 * \param value the value to be queried
 * \param f function pointer to the callback method that will handle the result set
 * \return zero for success, and non-zero if an error occurs
 */
int find_sensor_by_value(DBCONN *conn, sensor_value_t value, callback_t f){
	char sql[100];
	snprintf(sql,100,"SELECT * FROM %s WHERE sensor_value = %f",TO_STRING(TABLE_NAME),value);
	char*err_msg;
	int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
	if(rc!=SQLITE_OK){
		fprintf(stderr,"Falied to find the sensor with specific value\n");
		sqlite3_free(err_msg);
		return -1;
	}return 0;
}
/**
 * Write a SELECT query to return all sensor measurements of which the temperature exceeds 'value'
 * The callback function is applied to every row in the result
 * \param conn pointer to the current connection
 * \param value the value to be queried
 * \param f function pointer to the callback method that will handle the result set
 * \return zero for success, and non-zero if an error occurs
 */
int find_sensor_exceed_value(DBCONN *conn, sensor_value_t value, callback_t f){
        char sql[100];
        snprintf(sql,100,"SELECT * FROM %s WHERE sensor_value > %f",TO_STRING(TABLE_NAME),value);
        char*err_msg;
        int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
        if(rc!=SQLITE_OK){
                fprintf(stderr,"SQL error: %s\n",err_msg);
                sqlite3_free(err_msg);
                return -1;
        }return 0;
}

/**
 * Write a SELECT query to return all sensor measurements having a timestamp 'ts'
 * The callback function is applied to every row in the result
 * \param conn pointer to the current connection
 * \param ts the timestamp to be queried
 * \param f function pointer to the callback method that will handle the result set
 * \return zero for success, and non-zero if an error occurs
 */
int find_sensor_by_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f){
        char sql[100];
        snprintf(sql,100,"SELECT * FROM %s WHERE timestamp = %ld",TO_STRING(TABLE_NAME),ts);
        char*err_msg;
        int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
        if(rc!=SQLITE_OK){
                fprintf(stderr,"SQL error: %s\n",err_msg);
                sqlite3_free(err_msg);
                return -1;
        }return 0;
}

/**
 * Write a SELECT query to return all sensor measurements recorded after timestamp 'ts'
 * The callback function is applied to every row in the result
 * \param conn pointer to the current connection
 * \param ts the timestamp to be queried
 * \param f function pointer to the callback method that will handle the result set
 * \return zero for success, and non-zero if an error occurs
 */
int find_sensor_after_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f){
        char sql[100];
        snprintf(sql,100,"SELECT * FROM %s WHERE timestamp > %ld",TO_STRING(TABLE_NAME),ts);
        char*err_msg;
        int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
        if(rc!=SQLITE_OK){
                fprintf(stderr,"SQL error: %s\n",err_msg);
                sqlite3_free(err_msg);
                return -1;
        }return 0;
}

