/**
 * \author Zhewen Xue
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <time.h>
#define MAIN_ERR_HANDLER(condition,...)  \
  do {            \
    if ((condition))      \
    {         \
      DEBUG_PRINTF(1,"error condition \"" #condition "\" is true\n"); \
      __VA_ARGS__;        \
    }         \
  } while(0)
#define CONNMGR_FILE_ERR_HANDLER(fp)  \
   do {                        \
      if ( (fp) == NULL )               \
      {                       \
        perror("create the sensor_data_recv file failed.");         \
        exit( EXIT_FAILURE );           \
      }                       \
    } while(0)
#define LOGFILE_FILE_ERR_HANDLER(fp)  \
   do {                        \
      if ( (fp) == NULL )               \
      {                       \
        perror("Log file created failed.");         \
        exit( EXIT_FAILURE );           \
      }                       \
    } while(0)

#define SYSCALL_ERROR(err)                  \
    do {                        \
      if ( (err) == -1 )                \
      {                       \
        perror("Error executing syscall");      \
        exit( EXIT_FAILURE );           \
      }                       \
    } while(0)

#define CHECK_FOPEN(fp)  \
   do {                        \
      if ( ((FILE*)fp) == NULL )               \
      {                       \
        perror("File open failed.");         \
        exit( EXIT_FAILURE );           \
      }                       \
    } while(0)

#define CHECK_FCLOSE(err)  \
   do {                        \
      if ( err==-1 )               \
      {                       \
        perror("File closed failed.");         \
        exit( EXIT_FAILURE );           \
      }                       \
    } while(0)
#define CONNMGR_SELECT_ERROR(err)                 \
    do {                        \
      if ( (err) == 1 )                \
      {                       \
        perror("TCP_SOCKET_ERROR.");        \
        exit( EXIT_FAILURE );           \
      }else if((err)==2)                          \
      {                       \
        perror("TCP_ADDRESS_ERROR.");        \
        exit( EXIT_FAILURE );           \
      }                                           \
      else if((err)==3)                          \
      {                       \
        perror("The sensor value received has some problem.");        \
        exit( EXIT_FAILURE );           \
      }                                           \
      else if((err)==4)                          \
      {                       \
        perror("TCP CONNECTION CLOSED.");        \
        exit( EXIT_FAILURE );           \
      }                                           \
      else                        \
      {                       \
        perror("TCP memory error.");        \
        exit( EXIT_FAILURE );           \
      }\
    } while(0)
#define CHECK_MKFIFO(err) \
do {                        \
      if ( (err) == -1 )               \
      {                       \
        perror("make fifo failed.");         \
        exit( EXIT_FAILURE );           \
      }                       \
    } while(0)
#define CHECK_CLRFIFO(err) \
do {                        \
      if ( (err)==-1 )               \
      {                       \
        perror("clear fifo failed.");         \
        exit( EXIT_FAILURE );           \
      }                       \
    } while(0)
#ifdef DEBUG
#define DEBUG_PRINTF(condition,...)                 \
    do {                        \
       if((condition))                    \
       {                        \
      fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__); \
      fprintf(stderr,__VA_ARGS__);                \
       }                        \
    } while(0)
#else
#define DEBUG_PRINTF(...) (void)0
#endif
typedef uint16_t sensor_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;         // UTC timestamp as returned by time() - notice that the size of time_t is different on 32/64 bit machine
typedef uint16_t room_id_t;
typedef struct {
    sensor_id_t id;
    sensor_value_t value;
    sensor_ts_t ts;
} sensor_data_t;

#endif /* _CONFIG_H_ */
