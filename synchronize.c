#include "synchronize.h"

int suspend_flag;

void syn_init(){
	pthread_mutex_init(&syn_mutex,NULL);
	pthread_cond_init(&cond_prescan,NULL);
	pthread_cond_init(&cond_cuda,NULL);
	suspend_flag = 0;
}

void syn_suspend(pthread_cond_t cond){
	 pthread_cond_wait(&cond,&syn_mutex);
}


void syn_resume(pthread_cond_t cond){
	pthread_cond_signal(&cond);
}
