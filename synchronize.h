#ifndef SYCHRONIZE_H
#define SYCHRONIZE_H
#include <pthread.h>
pthread_mutex_t mutex;
pthread_cond_t cond_pre, cond_cuda;

void syn_suspend(pthread_cond_t cond);
void syn_resume(pthread_cond_t cond);

#endif
