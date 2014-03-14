#ifndef SYCHRONIZE_H
#define SYCHRONIZE_H
#include <pthread.h>
pthread_mutex_t syn_mutex, idle_mutex;
pthread_cond_t cond_cuda,cond_prescan,cond_schedule,cond_idle;

//
//	void syn_suspend(pthread_cond_t cond);
//	void syn_resume(pthread_cond_t cond);
//	void syn_init();

#endif
