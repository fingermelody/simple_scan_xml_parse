#include <mpi.h>
#include <time.h>
#include "stdafx.h"
#include "slaves.h"
#include "synchronize.h"
#include "MPI_MSG_TYPE.h"
#include "multi_read.h"
#include "Text.h"
#include "task_queue.h"

array* query(char* s){
	int i;
	MPI_Status status;
	int* results[slaves_num];
	array* res_arrays[slaves_num];
	for(i=0;i<slaves_num;i++){
		results[i] = (int*)malloc(sizeof(int));
		res_arrays[i] = (array*)malloc(sizeof(array));
		array_init(res_arrays[i]);
	}

	MPI_Status res_status[slaves_num];
	int count_all = 0;
	for(i=1;i<=slaves_num;i++){
		MPI_Send(s,strlen(s),MPI_CHAR,i,MSG_QUERY_REQUEST,MPI_COMM_WORLD);
		MPI_Probe(i,MSG_QUERY_RESPONSE,MPI_COMM_WORLD,&status);
		int count;
		MPI_Get_count(&status,MPI_INT,&count);
		results[i-1] = (int*)malloc(sizeof(int)*count);
		MPI_Recv(results[i-1],count,MPI_INT,i,MSG_QUERY_RESPONSE,MPI_COMM_WORLD,&res_status[i-1]);
		res_arrays[i-1]->data = results[i-1];
		res_arrays[i-1]->size = count;
		count_all += count;
	}
	int* a = (int*)malloc(sizeof(int)*count_all);
	int m=0;
	for(i=0;i<slaves_num;i++){
		int j;
		for(j=0;j<res_arrays[i]->size;j++,m++){
			a[m] = ((int*)(res_arrays[i]->data))[j];
		}
	}
	array* t = (array*)malloc(sizeof(array));
	array_init(t);
	t->data = a;
	t->size = count_all;
	return t;
}


/*
 * schedule thread will manage the xml file participate work, if there is an idle slave, we transfer
 * the xml file part we just get from pre-scan thread to the idle slave.
 * */
void* schedule(void* c_arg){

#ifdef MASTER_TIME_TEST
	struct timeval tim;
	gettimeofday(&tim,NULL);
	double start = tim.tv_sec + (tim.tv_usec/1000000.0);
#endif

	MPI_Request request[3];

	int indices[slaves_num],num_completed;
	int j;
	for(j=0;j<slaves_num;j++)
		request[j] = MPI_REQUEST_NULL;

	while(1){
		if(!multi_read_over() || !task_queue_is_empty()){
			parse_task* task = get_task();
#ifndef READ_TEST
			tag_info* host_tags_info = task->info;
			int tag_num = task->tag_num;
	//		change the struct Text and tag_info to bytes for transferring
			char* bytes_of_tags_info = (char*)host_tags_info;

			int dest = -1;
			while( dest <= 0){
				int i;
				for(i=1;i<=slaves_num;i++){

					if (idle_node[i]== 1) {
						idle_node[i] = 0;
						dest = i;
						break;
					}
				}
				if(dest <= 0){
					for(i=1;i<=slaves_num;i++)
						MPI_Irecv(&idle_node[i],1,MPI_INT,i,MSG_IDLE,MPI_COMM_WORLD,&request[i-1]);
					MPI_Waitsome(slaves_num,request,&num_completed,indices,MPI_STATUSES_IGNORE);
				}
			}
			idle_node[dest] = 0;//indicates this slave is busy now
	//		send partial file and tag information

			if(bytes_of_tags_info == NULL)
				printf("ERROR : tags info is null \n");
			MPI_Ssend(bytes_of_tags_info,tag_num*sizeof(tag_info),MPI_CHAR,dest,MSG_SEND_TAG_INFO,MPI_COMM_WORLD);

			free(task);
//			printf("%d,%d\n",multi_read_over(),task_queue_is_empty());
#endif
		}else{

#ifdef MASTER_TIME_TEST
		gettimeofday(&tim,NULL);
		double stop = tim.tv_sec + (tim.tv_usec/1000000.0);
		double dur = stop - start;
		printf(" master time report\n%ld--(%f,%f,%f)\n",task_counter,start,stop,dur);
#endif


#ifdef QUERY_TEST
		printf(" please input a tag name: \n");
		char s[31];
		scanf("%30s",s);
		int sum_count = 0;
		if(strlen(s)!=0){
			array* condition = (array*)malloc(sizeof(array));
			array_init(condition);
			parse_query(s,condition);
			array** results = (array*)malloc(sizeof(array*)*(condition->size));
			for(j=0;j<condition->size;j++){
				char* s = ((char**)(condition->data))[j];
				results[j] = query(s);
			}
			printf("qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq\n");
		}
#endif
#ifndef QUERY_TEST
			int i;
			char end;
			for(i=1;i<=slaves_num;i++)
				MPI_Send(&end,0,MPI_CHAR,i,MSG_EXIT,MPI_COMM_WORLD);
			return NULL;
#endif
		}

	}
	printf("master exit\n");
	return NULL;
}
