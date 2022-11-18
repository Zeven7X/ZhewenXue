#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <wait.h>
#include <sqlite3.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <resolv.h>
#include <sys/select.h>
#include "datamgr.h"
#include "sensor_db.h"
#include "connmgr.h"
#include "sbuffer.h"

#define FIFO_NAME "logFIFO"
#define LOGFILE_NAME "gateway.log"
#define STRMGR1 " Connection to SQL server established.\n"
#define thread_barrier 2
sbuffer_t* shared_buffer;
int connmgr_listening = 1;
static pthread_barrier_t barrier;
void log_process(void);
void* method_connmgr(void *arg)
{
    printf("connmgr thread create.\n");
//    int *server_port = (int*)arg;
    connmgr_listen(*((int*)arg),&shared_buffer);
    connmgr_listening = 0;

    connmgr_free();
    printf("CONNMGR LISTENING CLOSED  Datamgr thread finished.\n\n\n");
    return NULL;

}
//this function fetch sensor data,
void* method_datamgr(void *arg)
{
    printf("datamgr thread create.\n");
    FILE *fp_sensor_map;
    fp_sensor_map = fopen("room_sensor.map","r");
    CHECK_FOPEN(fp_sensor_map);
    //first to parse the map file and create a dplist in datamgr.c
    datamgr_read_room_file(fp_sensor_map);
    CHECK_FCLOSE(fclose(fp_sensor_map));
    //then ,to parse the data in sbuffer.
    while(connmgr_listening!=0||sbuffer_size(shared_buffer)>0){
        datamgr_parse_sensor_files(shared_buffer,&barrier);
        struct timeval sleeptime;
        sleeptime.tv_sec = 0;
        sleeptime.tv_usec=5000;
        select(0,0,0,0,&sleeptime);
    }
    datamgr_free();
    printf("room_sensor.map closed. Datamgr thread finished.\n");
    return NULL;
}

void* method_strmgr(void *arg)
{
    printf("strmgr thread create.\n");
    sqlite3 *db;
    int times = 0;
    db = init_connection(1);
    //if open fail, blahblahblah
    while(db == NULL){
        times++;
        if(times<=3){
            sleep(5);
        }else{
            connmgr_free();
            datamgr_free();
            sbuffer_free(&shared_buffer);
            printf("try to connect the database, but fail three times, close the gateway.\n");
            exit(EXIT_FAILURE);
        }
        db = init_connection(0);
    }
    times = 0;
    int goon = 1;
    int result ;
    int loop = 1;
//    sensor_data_t *sensorData ;
    // I think this loop should contain the method related to TIMEOUT
    while(goon){

        if(loop==1&&connmgr_listening==0&&sbuffer_size(shared_buffer)==0){
            disconnect(db);
            printf("database closed.\n");
            loop = 0;
            goon = 0;
        }if(loop == 1) {
            result = insert_sensor(db, shared_buffer,&barrier);
            if (result == 0) {
                printf("Successfully write one node to database.\n");
            }
            struct timeval sleeptime;
            sleeptime.tv_sec = 0;
            sleeptime.tv_usec = 5000;
            select(0, 0, 0, 0, &sleeptime);
        }

    }
    printf("data manger runs out.\n");
    char * info=(char *)malloc(50*sizeof(char));
    strcpy(info," Unable to connect to SQL server.\n");
    write_to_logfile(info);
    free(info);
    return NULL;

}

int main(int argc, char *argv[])
{
    pid_t child_pid;//log process
    child_pid = fork();
    if((child_pid)==-1){
        perror("create child_pid fail.\n");
        exit(EXIT_FAILURE);
    }
    if(child_pid==0){
        log_process();
    }else{
        printf("parent process, pid = %d.\n",getpid());
        int result,server_port;//an interger for error handling
        if (access(FIFO_NAME, F_OK) == -1) {

            result = mkfifo(FIFO_NAME, 0666);//every one can read and write
            CHECK_MKFIFO(result);
        }
        printf("FIFO creation succeeds!\n");
        if (argc != 2){
            printf("You should write the command in the right way.\n");
            exit(EXIT_FAILURE);
        }else{
            server_port = atoi(argv[1]);
        }
        printf("The server port number is %d\n",server_port);
        int presult;
        pthread_t thread_connmgr;
        pthread_t thread_datamgr;
        pthread_t thread_strmgr;
        pthread_barrier_init(&barrier,NULL,thread_barrier);
        if(sbuffer_init(&shared_buffer)!=0)
        {
            printf("Couldn't initialize the shared buffer!\n");
            exit(EXIT_FAILURE);
        }

        presult=pthread_create(&thread_connmgr, NULL, method_connmgr, &server_port);
        MAIN_ERR_HANDLER(presult!=0);

        presult=pthread_create(&thread_datamgr, NULL,method_datamgr,NULL);
        MAIN_ERR_HANDLER(presult!=0);

        presult=pthread_create(&thread_strmgr, NULL,method_strmgr,NULL);
        MAIN_ERR_HANDLER(presult!=0);


        presult=pthread_join(thread_connmgr,NULL);
        MAIN_ERR_HANDLER(presult!=0);
        printf("thread_connmgr joined.\n");
        presult=pthread_join(thread_datamgr,NULL);
        MAIN_ERR_HANDLER(presult!=0);
        printf("thread_datamgr joined.\n");
        presult=pthread_join(thread_strmgr,NULL);
        MAIN_ERR_HANDLER(presult!=0);
        printf("thread_strmgr joined.\n");
        pthread_barrier_destroy(&barrier);
//        kill(child_pid,SIGUSR1);
        if(sbuffer_free(&shared_buffer)!=0)
        {
            printf("Couldn't free the shared buffer!\n");
            exit(EXIT_FAILURE);
        }
//        waitpid(child_pid,NULL,0);
        printf("sbuffer free.\n");

        int log_exit_status;
        // wait log process exit
        pid_t pid = wait(&log_exit_status);
        if (WIFEXITED(log_exit_status))
        {
            printf("Log process %d terminated with exit status %d\n", pid, WEXITSTATUS(log_exit_status));
        }
        else
        {
            printf("Log process %d terminated abnormally\n", pid);
        }
        if (access(FIFO_NAME, F_OK) == 0) {
            CHECK_CLRFIFO(remove(FIFO_NAME));
        }
    }
    return 0;
}



void log_process(void){
    printf("child process, pid = %d, ppid = %d.\n",getpid(),getppid());
    char *result;
    char recv_buf[120];
    int res=0;
    if (access(FIFO_NAME, F_OK) == -1) {

        res = mkfifo(FIFO_NAME, 0666);//every one can read and write
        CHECK_MKFIFO(res);
    }
    printf("FIFO creation succeeds!\n");
    FILE *fb=fopen(FIFO_NAME, "r");
    CHECK_FOPEN(fb);
    FILE *logfile = fopen("gateway.log", "w");
    CHECK_FOPEN(logfile);
        //write the info to gateway.log
    result = fgets(recv_buf, 120, fb);
    if (result != NULL)
    {
        fprintf(logfile, "%s", recv_buf);}
    while (result != NULL){
        result = fgets(recv_buf, 120, fb);
        if (result != NULL)
        {
            fprintf(logfile, "%s", recv_buf);}

    }

    // close file and handle exit status code
    CHECK_FCLOSE(fclose(logfile));
    CHECK_FCLOSE(fclose(fb));
    CHECK_CLRFIFO(remove(FIFO_NAME));
    printf("Now Log process goes out !!\n");
}

