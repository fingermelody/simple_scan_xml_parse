#include <mpi.h>
#include "stdafx.h"
#include "slaves.h"
#include "synchronize.h"
#include "MPI_MSG_TYPE.h"
#include "simple_state_machine.h"
#include "Text.h"
/*
 * search an idle slave
 * */
int get_idle_node(){
//	pthread_mutex_lock(&idle_mutex);
	MPI_Request request;
	int r = -1;
	while( r <= 0){
		MPI_Irecv(&idle_node[1],1,MPI_INT,1,MSG_IDLE,MPI_COMM_WORLD,&request);
		int i;
		for(i=1;i<=slaves_num;i++){
			printf("slave %d, now is %d \n",i,idle_node[i]);
			if (idle_node[i]== 1) {
				idle_node[i] = 0;
				r = i;
				return r;
			}
		}
		sleep(1);
//		struct timespec ts;
//		struct timeval tp;
//		gettimeofday(&tp,NULL);
//		ts.tv_sec  = tp.tv_sec;
//		ts.tv_nsec = tp.tv_usec * 1000;
//		ts.tv_sec += 1;
//		pthread_cond_timedwait(&cond_idle,&idle_mutex,&ts);
//		pthread_mutex_unlock(&idle_mutex);
	}
	return r;
}

/*
 * schedule thread will manage the xml file participate work, if there is an idle slave, we transfer
 * the xml file part we just get from pre-scan thread to the idle slave.
 * */
void* schedule(void* c_arg){
	MPI_Request request;
	MPI_Status status;
	while(1){

		//waiting the pre-scan thread to make a xml file part that can be parsed by the slaves.
		pthread_cond_wait(&cond_schedule, &syn_mutex);
		simple_parse_arg *arg = (simple_parse_arg*)c_arg;
		tag_info* host_tags_info = *(arg->s_tags_ready);

//		printf("master tag 20's info is following:\n");
//		printf("id:%d length:%d location:%d\n",host_tags_info[20].id,host_tags_info[20].lengh,host_tags_info[20].location);

		Text* text = *(arg->text_read);
//		change the struct Text and tag_info to bytes for transferring
		char* bytes_of_tags_info = (char*)host_tags_info;
		char* bytes_of_text = (char*)text;

//		int dest = get_idle_node();// search a idle node
		int dest = -1;
		while( dest <= 0){

			int i;
			for(i=1;i<=slaves_num;i++){
				printf("slave %d, now is %d \n",i,idle_node[i]);
				if (idle_node[i]== 1) {
					idle_node[i] = 0;
					dest = i;
					break;
				}
			}
			if(dest <= 0){
				MPI_Irecv(&idle_node[1],1,MPI_INT,1,MSG_IDLE,MPI_COMM_WORLD,&request);
				MPI_Irecv(&idle_node[2],1,MPI_INT,2,MSG_IDLE,MPI_COMM_WORLD,&request);
				MPI_Irecv(&idle_node[3],1,MPI_INT,3,MSG_IDLE,MPI_COMM_WORLD,&request);
//				MPI_Wait(&request,&status);
			}
		}
		idle_node[dest] = 0;//indicates this slave is busy now
//		send partial file and tag information
		printf("sending partial file to slaves %d...\n",dest);
		if(text == NULL)
			printf("ERROR : host_read_string is null\n");
		if(bytes_of_tags_info == NULL)
			printf("ERROR : tags info is null \n");
		int text_length = text_size(text);
		MPI_Ssend(bytes_of_text,sizeof(Text),MPI_CHAR,dest,MSG_TEXT,MPI_COMM_WORLD);
		MPI_Ssend(bytes_of_tags_info,TAGS_PER_TIME*sizeof(tag_info),MPI_CHAR,dest,MSG_SEND_TAG_INFO,MPI_COMM_WORLD);
		printf("master send data successfully \n");
		pthread_cond_signal(&cond_prescan);
//		if(file_read_over){
//			int i;
//			for(i=1;i<=slaves_num;i++)
//				MPI_Send(&i,0,MPI_INT,i,MSG_EXIT,MPI_COMM_WORLD);
//			break;
//		}
		int idle_state;
		int idle_rank;
	}
	return NULL;
}


/*
void* monitor(void* argc){
	MPI_Status status;
	while(1){
		MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
		if(status.MPI_SOURCE == 0) continue;
		switch(status.MPI_TAG){
		case MSG_IDLE:
		{
			int idle_state;
			int idle_rank;
			MPI_Recv(&idle_state,1,MPI_INT,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status);
			idle_node[status.MPI_SOURCE] = idle_state;
//			pthread_mutex_lock(&idle_mutex);
//			pthread_cond_signal(&cond_idle);
//			pthread_mutex_unlock(&idle_mutex);
			break;
		}
		case MSG_QUERY_RESPONSE:
		{
			array* id_array = (array*)malloc(sizeof(array));
			int* result[slaves_num+1];
			int count[slaves_num+1];
			MPI_Get_count(&status,MPI_INT,&(count[status.MPI_SOURCE]));
			result[status.MPI_SOURCE] = (int*)malloc(sizeof(int)*count[status.MPI_SOURCE]);
			MPI_Recv(result[status.MPI_SOURCE],count[status.MPI_SOURCE],MPI_INT,status.MPI_SOURCE,4,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			int i;
			for(i=1;i<=slaves_num;i++)
				count[0] += count[i];
			result[0] = (int*)malloc(sizeof(int)*count[0]);
			int* p = result[0];
			for(i=1;i<=slaves_num;i++){
				memcpy(p,result[i],count[i]);
				free(result[i]);
				p = &(p[count[i]]);
			}
			id_array->data = result[0];
			id_array->size = count[0];
			break;
		}
		}
	}
	return NULL;
}
*/
