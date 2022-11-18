//
// Created by zhewen on 27.12.20.
//

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
#include "datamgr.h"
#include "sensor_db.h"
#include "connmgr.h"
#include "sbuffer.h"

int main(int argc, char *argv[]){
    printf("datamgr thread create.\n");
    FILE *fp_sensor_map;
    fp_sensor_map = fopen("room_sensor.map","rb");
    CHECK_FOPEN(fp_sensor_map);
    datamgr_read_room_file(fp_sensor_map);
    datamgr_free();
    printf("get it.\n");
    return 0;
}

