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
#define MULTI_READ_TIME_TEST
#define MASTER_TIME_TEST
//#define TASK_ADD_TIME_TEST
//#define TASK_GET_TIME_TEST
#define TASK_NUM_TEST
//#define READ_TEST
//#define SINGLE_THREAD_TIME_TEST
#define HOST_TIME_TEST
#define CUDA_TIME_TEST
#define SLAVE_TIME_TEST
//#define QUERY_TEST
int di;

clock_t read_start, read_finish;
clock_t task_add_time,old_task_add_time;
clock_t task_get_time,old_task_get_time;
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
