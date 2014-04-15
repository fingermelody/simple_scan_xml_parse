#include "task_queue.h"

void task_queue_init(){
	pthread_mutex_init(&mutex_task,NULL);
	pthread_mutex_init(&mutex_full,NULL);
	pthread_mutex_init(&mutex_empty,NULL);
	pthread_cond_init(&cond_full,NULL);
	pthread_cond_init(&cond_empty,NULL);

	tasks_num =0;
	memset(tasks,0,sizeof(parse_task)*MAX_TASK_NUM);
}

int add_task(parse_task* task){
	pthread_mutex_lock(&mutex_task);
	if(tasks_num >= MAX_TASK_NUM){
		pthread_cond_wait(&cond_full,&mutex_task);
	}
	tasks[tasks_num++] = task;
	pthread_mutex_unlock(&mutex_task);
	if(tasks_num == 1) pthread_cond_signal(&cond_empty);

	return 0;
}

parse_task* get_task(){
	pthread_mutex_lock(&mutex_task);
	if(tasks_num <= 0){
		pthread_cond_wait(&cond_empty,&mutex_task);
	}
	parse_task* task;
	task = tasks[--tasks_num];
	pthread_mutex_unlock(&mutex_task);
	if(tasks_num == MAX_TASK_NUM-1) pthread_cond_signal(&cond_full);
	return task;
}

int task_queue_is_empty(){
	return (tasks_num==0)?1:0;
}
