/*
 * debug.h
 *
 *  Created on: Mar 10, 2014
 *      Author: jerry
 */

#ifndef DEBUG_H_
#define DEBUG_H_
#include "time.h"
//#define L_DEBUG 1
#define TIME_TEST
//#define TASK_ADD_TIME_TEST
int di;

clock_t read_start, read_finish;
clock_t task_add_time,old_task_add_time;
long task_counter;

#define GDB_WAIT_ATTACH() do{				\
    char hostname[256];						\
    gethostname(hostname, sizeof(hostname));\
    printf("PID %d on %s ready for attach\n", getpid(), hostname);\
    fflush(stdout);							\
    while (0 == di)							\
        sleep(5);							\
}while(0)

#endif /* DEBUG_H_ */
