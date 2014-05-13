#include "task_queue.h"
#include "debug.h"
void task_queue_init(){
	pthread_mutex_init(&mutex_task,NULL);
	pthread_mutex_init(&mutex_full,NULL);
	pthread_mutex_init(&mutex_empty,NULL);
	pthread_cond_init(&cond_full,NULL);
	pthread_cond_init(&cond_empty,NULL);

	tasks = (parse_task**)malloc(sizeof(parse_task*)*MAX_TASK_NUM);
	tasks_num =0;
	memset(tasks,0,sizeof(parse_task*)*MAX_TASK_NUM);
}

int add_task(parse_task* task){

	pthread_mutex_lock(&mutex_task);
	if(tasks_num >= MAX_TASK_NUM){
		pthread_cond_wait(&cond_full,&mutex_task);
	}
	tasks[tasks_num++] = task;
#ifdef TASK_ADD_TIME_TEST
	task_add_time = clock();
	float task_crt_dur = (float)(task_add_time - old_task_add_time)/CLOCKS_PER_SEC;
	printf("adding a task costs %f\n",task_crt_dur);
	old_task_add_time = task_add_time;
#endif

#ifdef TASK_NUM_TEST
	task_counter++;
#endif

	pthread_mutex_unlock(&mutex_task);
	if(tasks_num >= 1) pthread_cond_signal(&cond_empty);

	return 0;
}

parse_task* get_task(){
	pthread_mutex_lock(&mutex_task);
	if(tasks_num <= 0){
		pthread_cond_wait(&cond_empty,&mutex_task);
	}
	parse_task* task;
	task = tasks[--tasks_num];
#ifdef TASK_GET_TIME_TEST
	task_get_time = clock();
	float task_crt_dur = (float)(task_add_time - old_task_add_time)/CLOCKS_PER_SEC;
	printf("getting a task costs %f\n",task_crt_dur);
	old_task_get_time = task_get_time;
#endif
	pthread_mutex_unlock(&mutex_task);
	if(tasks_num == MAX_TASK_NUM-1) pthread_cond_signal(&cond_full);
	return task;
}

int task_queue_is_empty(){
//	pthread_mutex_lock(&mutex_task);
//	int result = (tasks_num==0)?1:0;
//	pthread_mutex_unlock(&mutex_task);
//	return result;
	return (tasks_num==0)?1:0;
}
