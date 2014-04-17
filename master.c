#include <mpi.h>
#include <time.h>
#include "stdafx.h"
#include "slaves.h"
#include "synchronize.h"
#include "MPI_MSG_TYPE.h"
#include "simple_state_machine.h"
#include "Text.h"
#include "task_queue.h"


/*
 * schedule thread will manage the xml file participate work, if there is an idle slave, we transfer
 * the xml file part we just get from pre-scan thread to the idle slave.
 * */
void* schedule(void* c_arg){
	clock_t start, finish;
	MPI_Request request[3];
	MPI_Status status;
	int indices[slaves_num],num_completed;
	int j;
	for(j=0;j<slaves_num;j++)
		request[j] = MPI_REQUEST_NULL;

	start = clock();
	while(1){
		if(file_read_over == 0){
			parse_task* task = get_task();

	#ifdef L_DEBUG
			printf("get a task ! PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP\n");
	#endif
			tag_info* host_tags_info = task->info;
			int tag_num = task->tag_num;
//			printf("master tag 20's info is following:\n");
//			printf("id:%d length:%d location:%d\n",host_tags_info[20].id,host_tags_info[20].length,host_tags_info[20].location);

	//		change the struct Text and tag_info to bytes for transferring
			char* bytes_of_tags_info = (char*)host_tags_info;

			int dest = -1;
			while( dest <= 0){
				int i;
				for(i=1;i<=slaves_num;i++){
#ifdef L_DEBUG
					printf("slave %d, now is %d \n",i,idle_node[i]);
#endif
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
#ifdef L_DEBUG
			printf("master send data successfully \n");
#endif
		}
		//
		if(file_read_over == 1 && task_queue_is_empty)
		{
#ifdef TIME_TEST
			finish = clock();
			float duration = (float)(finish - start)/CLOCKS_PER_SEC;
			float read_time = (float)(read_finish - read_start)/CLOCKS_PER_SEC;
			printf("whole time report\n%d--(%f,%f)\n please input a tag name:\n",task_counter,duration,read_time);
#endif
			char s[31];
			scanf("%30s",s);
			if(strlen(s)!=0){
				int i;
				int* results[slaves_num];
				array* res_arrays[slaves_num];
				MPI_Status res_status[slaves_num];
				for(i=1;i<=slaves_num;i++){
					MPI_Send(s,strlen(s),MPI_CHAR,i,MSG_QUERY_REQUEST,MPI_COMM_WORLD);
					MPI_Probe(i,MSG_QUERY_RESPONSE,MPI_COMM_WORLD,&status);
					int count;
					MPI_Get_count(&status,MPI_INT,&count);
					results[i-1] = (int*)malloc(sizeof(int)*count);
					MPI_Recv(results[i-1],count,MPI_INT,i,MSG_QUERY_RESPONSE,MPI_COMM_WORLD,&res_status[i-1]);
					res_arrays[i-1]->data = results[i-1];
					res_arrays[i-1]->size = count;
				}
			}
			printf("%s\n",s);
		}
	}
	return NULL;
}
