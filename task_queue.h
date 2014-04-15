/*
 * task_queue.h
 *
 *  Created on: Apr 12, 2014
 *      Author: jerry
 */

#ifndef TASK_QUEUE_H_
#define TASK_QUEUE_H_

#include <pthread.h>
#include "tag.h"
#include "Text.h"

#define MAX_TASK_NUM 1000


typedef struct _parse_task{
	tag_info* info;
	int  tag_num;
}parse_task;

pthread_mutex_t mutex_task,mutex_full,mutex_empty;
pthread_cond_t cond_full, cond_empty;
int tasks_num;

parse_task* tasks[MAX_TASK_NUM];

parse_task* get_task();
int add_task(parse_task* task);
int task_queue_is_empty();
#endif /* TASK_QUEUE_H_ */
