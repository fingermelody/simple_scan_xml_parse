#ifndef SYCHRONIZE_H
#define SYCHRONIZE_H
#include <pthread.h>
pthread_mutex_t syn_mutex;
pthread_cond_t cond_cuda;
pthread_cond_t cond_prescan;

//
//	void syn_suspend(pthread_cond_t cond);
//	void syn_resume(pthread_cond_t cond);
//	void syn_init();

#endif
