/*
 * debug.h
 *
 *  Created on: Mar 10, 2014
 *      Author: jerry
 */

#ifndef DEBUG_H_
#define DEBUG_H_

int di;

#define GDB_WAIT_ATTACH() do{				\
    char hostname[256];						\
    gethostname(hostname, sizeof(hostname));\
    printf("PID %d on %s ready for attach\n", getpid(), hostname);\
    fflush(stdout);							\
    while (0 == di)							\
        sleep(5);							\
}while(0)

#endif /* DEBUG_H_ */
