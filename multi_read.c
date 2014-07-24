#include "stdafx.h"
#include "simple_parser.h"
#include "pthread.h"
#include "reader.h"
#include "task_queue.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "TagArray.h"
#define MAX_THREAD_NUM 50
pthread_t thread_pool[MAX_THREAD_NUM];
parse_area parse_parts[MAX_THREAD_NUM];

int file_read_over;
Tag_Array* all_tags;
pthread_mutex_t multi_read_mutex;

void* single_read(void* arg){
#ifdef SINGLE_THREAD_TIME_TEST
	struct timeval start, stop;
	gettimeofday(&start,NULL);
	double start_t = start.tv_sec + start.tv_usec/1000000.0;
#endif
	reader* freader = (reader*)arg;
	reader_read_and_handle(freader);
	reader_seam(freader);


#ifdef SINGLE_THREAD_TIME_TEST
	gettimeofday(&stop,NULL);
	double stop_t = stop.tv_sec + stop.tv_usec/1000000.0;
	printf("single time report:(%.6lf,%.6lf,%.6lf)\n",start_t,stop_t,stop_t-start_t);
#endif
	return NULL;
}

void multi_read(char* filename, int thread_num){
	file_read_over = 0;
	if(thread_num <= 0) return;
#ifdef MULTI_READ_TIME_TEST
	struct timeval tim;
	gettimeofday(&tim,NULL);
	double m_start = tim.tv_sec + (tim.tv_usec/1000000.0);
#endif
	FILE *pf = fopen(filename,"r");
	if(!pf) printf("open file failed!\n");
	fseek(pf,0,SEEK_END);
	size_t length = ftell(pf);
	size_t width = length/thread_num;
	fclose(pf);
	int i;
	simple_parser** parsers = (simple_parser**)malloc(sizeof(simple_parser*)*thread_num);
	reader** readers = (reader**)malloc(sizeof(reader*)*thread_num);
	for(i=0;i<thread_num;i++){
		parse_parts[i].base = i*width;
		parse_parts[i].length = width;
	}
	parse_parts[thread_num-1].length = length - parse_parts[thread_num-1].base;

	for(i=0;i<thread_num;i++){
		parsers[i] = (simple_parser*)malloc(sizeof(simple_parser));
		readers[i] = (reader*)malloc(sizeof(reader));
		unsigned long id_base = (TAG_ARRAY_MAX_SIZE/thread_num) * i;
		parser_init(parsers[i],NULL,id_base);
		reader_init(readers[i],parsers[i],filename,parse_parts[i].base,parse_parts[i].length);
	}

	for(i=0;i<thread_num;i++){
		pthread_create(&(thread_pool[i]),NULL,single_read,(void*)readers[i]);
	}
	for(i=0;i<thread_num;i++){
		pthread_join(thread_pool[i],NULL);
	}
	file_read_over = 1;
#ifdef MULTI_READ_TIME_TEST
	gettimeofday(&tim,NULL);
	double m_stop = tim.tv_sec + (tim.tv_usec/1000000.0);
	double dur = m_stop - m_start;
	printf("****************************************\n");
	printf("multi_read over, (%.6lf，%.6lf，%.6lf)\n",m_start,m_stop, dur);
	printf("****************************************\n");
#endif
//	all_tags = parser_merge(parsers,thread_num);
}

int multi_read_over(){
	return file_read_over;
}
