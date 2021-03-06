#include "synchronize.h"
#include "stack.h"
#include "tag.h"
#include "node.h"
#include "stdafx.h"
#include "cuda_runtime.h"
#include "cuda.h"
#include "device_launch_parameters.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <mpi.h>
#include "Text.h"
#include "tag.h"
#include "tools/hash_map.h"
#include "MPQ_hash.h"
#include "slaves.h"
#include "MPI_MSG_TYPE.h"
#include "debug.h"
#include "simple_scan_parser.h"
#include "multi_read.h"
#include "simple_parser.h"

#define TagNameAddChar(tag, c) do{			\
	tag->name[tag->nameCharIndex%MAX_STRING] = c;		\
	tag->nameCharIndex++;					\
}while(0)

#define TagAddAttr(tag, attr) do{			\
	tag->attributes[tag->attrIndex % MAX_ATTRIBUTE_NUM]=attr;	\
	tag->attrIndex++;						\
}while(0)

#define AttrNameAddChar(attr, c) do{		\
	attr->name[attr->nameCharIndex%MAX_STRING] = c;    \
	attr->nameCharIndex++;					\
}while(0)

#define AttrValueAddChar(attr,c) do{		\
	attr->value[attr->valueCharIndex%MAX_STRING] = c;  \
	attr->valueCharIndex++;					\
}while(0)

#define HANDLE_ERROR( err ) (HandleError( err, __FILE__, __LINE__ ))

//__constant__ unsigned char pBuffer[16*16*16*16*LENGTH];

static void HandleError( cudaError_t err,
                         const char *file,
                         int line ) {
    if (err != cudaSuccess) {
        printf( "%s in %s at line %d\n", cudaGetErrorString( err ),
                file, line );
        exit( EXIT_FAILURE );
    }
}
char file_path[100];
typedef struct{
	char **host_read_string;
	Tag **host_tags_info;
}cuda_arg;

extern "C"{
	void syn_init();
	void syn_suspend(pthread_cond_t cond);
	void syn_resume(pthread_cond_t cond);
	void hash_map_init(hash_map *map, size_t capacity, hash_map_comparator comparator, hash_map_hash_func hash_func);
	void hash_map_insert(hash_map *map, void *key, void *value);
	void prepareCryptTable();
	size_t hash_map_MPQ_hash_func(const void *key, size_t capacity);
	int MPQ_comparator(const void* l, const void *r);
	void* slave_query_worker(void* argc);
	void* query(void* argc);
	void* schedule(void* argc);
	void* monitor_slaves(void* argc);
	void text_init(Text* text);
	void text_add_char(Text* text, char c);
	void slave_query(int count);
	void task_queue_init();
	void multi_read(char* filename,int thread_num);
}

/*
 * GPU kernel. parse the xml file to tags' detail
 * @param: tags_info, tags' basic structure information, this rountine will complete the information in this Tag list.
 * @param: chars of partial xml file got from master.
 * */
__global__ void parse_tag(tag_info* tags_info, char* s, long base,long length, Tag* tags){

	int id = threadIdx.x;
	tag_info info = tags_info[id];

	Tag* t = &(tags[id]);
	long len = info.length;
	if(len == 0) return;

	t->info = info;


	long curIndex = info.cuda_parse_index;
	long end = curIndex + len;
//	if(s[curIndex]=='<')
//		t->name[0] = 'R';
//	else
//		t->name[0] = 'E';

	enum _state st = st_idle;
	attribute tmp_attr;



	while(curIndex  < end){
		char cur = NULL;
		if(curIndex < 0) break;
		cur = s[curIndex];

		if(cur == NULL) break;

		switch(st){
			case st_idle:{
				//start state
				if('<' == cur){
					st = st_lt;
					break;
				}
				break;
			}//end of case st_idle
			case st_lt:{
				if('>' == cur){
					st = st_idle;
					break;
				}
				//if cur is other character, this node is a start tag
				TagNameAddChar(t,cur);
				st = st_start_tag;
				break;
			}
			case st_start_tag:{//record tag name here
				if(' ' == cur){
					memset(&tmp_attr,0,sizeof(attribute));
					st = st_attribute_pre;
				}else if('>' == cur){
					st = st_idle;
				}else{// else this char belongs to tag name
					TagNameAddChar(t,cur);
				}
				break;
			}//end of state st_start_tag
			case st_attribute_pre:{
				if(' ' == cur){
					//ignore space
				}else if('>' == cur){// normal end
					st = st_idle;
				}else{
					AttrNameAddChar((&tmp_attr),cur);
					st = st_attribute_name;
				}
				break;
			}
			case st_attribute_name:{
				if('>' == cur){
					st = st_idle;
				}else if('=' == cur){
					st = st_attribute_value_pre;
				}else{
					AttrNameAddChar((&tmp_attr),cur);
				}
				break;
			}
			case st_attribute_value_pre:{
				if(' ' == cur){
					//ignore space
				}else if('>' == cur){
					st = st_idle;
				}else if(('"'==cur)||('\''==cur)){
					st = st_attribute_value;
				}
				break;
			}
			case st_attribute_value:{
				if(('"'==cur)||('\''==cur)){
					TagAddAttr(t,tmp_attr);
					//free(tmp_attr);
					st = st_attribute_pre;
				}else{
					AttrValueAddChar((&tmp_attr),cur);
				}
				break;
			}
			default: break;
		}//end of switch

		curIndex ++;
	}//end of while.


	__syncthreads();
}

/*
 * slave thread will receive the partial XML file from master, and parse the xml into tags details,
 * including tag name, attribute,then uses these details to build tag invert index, which includes
 * the tag name as the key and the tag id as the value.
 * */
void* slave_parse(void* argc){

#ifdef TIME_TEST
	float whole_cuda_parse = 0.0;
	long all_tasks_length = 0;
	long slave_tasks = 0;
#endif
#ifdef SLAVE_TIME_TEST
	struct timeval slave_start, slave_stop;
	gettimeofday(&slave_start,NULL);
	double start_t1 = slave_start.tv_sec + (slave_start.tv_usec/1000000.0);
#endif
	MPI_Status s_prob,s_recv_info;
	size_t inverted_index_map_size = 134217728; //2^27
	tags_inverted_index = (hash_map*)malloc(sizeof(hash_map));
	hash_map_init(tags_inverted_index,inverted_index_map_size,MPQ_comparator,hash_map_MPQ_hash_func);
	prepareCryptTable();//CryptTable is for the MPQ hash algorithm

	char* bytes_of_text = (char*)malloc(sizeof(Text));
	memset(bytes_of_text,0,sizeof(Text));
	char* tag_info_bytes; //buffer for receiving Tags' information in the partial xml file
	tag_info_bytes =(char*)malloc(TAGS_PER_TIME*sizeof(tag_info));
	memset(tag_info_bytes,0,sizeof(tag_info)*TAGS_PER_TIME);

	int pf;
	pf = open(file_path,S_IRUSR);

	int slave_rank;
	int text_ready = 0, tag_info_ready = 0;
	int tags_num;
	MPI_Comm_rank(MPI_COMM_WORLD,&slave_rank);

	tag_info* host_tag_infos;
	char* host_read_string;
	int cuda_index;
	int over = 0;
	while(!over){
		MPI_Probe(0,MPI_ANY_TAG,MPI_COMM_WORLD,&s_prob);
#ifdef L_DEBUG
		printf("slave receive a message %d\n",s_prob.MPI_TAG);
#endif
		int count;
		int i;
		MPI_Get_count(&s_prob,MPI_CHAR,&count);
		switch(s_prob.MPI_TAG){

		case MSG_SEND_TAG_INFO:

		slave_tasks ++;

			MPI_Recv(tag_info_bytes,count,MPI_CHAR,0,MSG_SEND_TAG_INFO,MPI_COMM_WORLD,&s_recv_info);
			tag_info_ready = 1;
			tags_num = count/sizeof(tag_info);
			host_tag_infos = (tag_info*)tag_info_bytes;

			cuda_index = 0;
			for(i=0;i<tags_num;i++){
				host_tag_infos[i].cuda_parse_index = cuda_index;
				cuda_index += host_tag_infos[i].length;
			}
			host_read_string = (char*)malloc(cuda_index);

			for(i=0;i<tags_num;i++){
				lseek(pf,host_tag_infos[i].location,SEEK_SET);
				read(pf,host_read_string+host_tag_infos[i].cuda_parse_index,host_tag_infos[i].length);
			}

//			printf("%10s\n",host_read_string);
			text_ready = 1;

			break;
		case MSG_QUERY_REQUEST:
			slave_query(count);
			break;
		case MSG_EXIT:
//			int end;
//			MPI_Recv(&end,0,MPI_INT,0,MSG_EXIT,MPI_COMM_WORLD,&status);
			printf("EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n");
//			pthread_exit(NULL);
			over = 1;
			break;
//
		}

		if(text_ready && tag_info_ready){

			char* device_string_to_parse;
			Tag* device_tags;
			tag_info* device_tag_info;
#ifdef CUDA_TIME_TEST
			cudaEvent_t cuda_parse_start, cuda_parse_finish;
			cudaEventCreate(&cuda_parse_start);
			cudaEventCreate(&cuda_parse_finish);
			cudaEventRecord(cuda_parse_start,0);
#endif
			HANDLE_ERROR(cudaMalloc((void**)&device_string_to_parse,cuda_index));
			HANDLE_ERROR(cudaMemset(device_string_to_parse,0,cuda_index));
			HANDLE_ERROR(cudaMalloc((void**)&device_tags,sizeof(Tag)*TAGS_PER_TIME));
			HANDLE_ERROR(cudaMemset(device_tags,0,sizeof(Tag)*TAGS_PER_TIME));
			HANDLE_ERROR(cudaMalloc((void**)&device_tag_info,sizeof(tag_info)*TAGS_PER_TIME));
			HANDLE_ERROR(cudaMemset(device_tag_info,0,sizeof(tag_info)*TAGS_PER_TIME));
			HANDLE_ERROR(cudaMemcpy(device_string_to_parse,host_read_string,cuda_index,cudaMemcpyHostToDevice));
			HANDLE_ERROR(cudaMemcpy(device_tag_info,host_tag_infos,sizeof(tag_info)*TAGS_PER_TIME,cudaMemcpyHostToDevice));


//			printf("tags num is %d\n",tags_num);
//			base = strlen(host_read_string);

			parse_tag<<<1,tags_num>>>(device_tag_info, device_string_to_parse,0,strlen(host_read_string),device_tags);
			HANDLE_ERROR(cudaPeekAtLastError());
			HANDLE_ERROR(cudaDeviceSynchronize());


			Tag* host_tags_buffer = (Tag*)malloc(sizeof(Tag)*TAGS_PER_TIME);//buffer for restoring the tag parse result.
			memset(host_tags_buffer,0,sizeof(Tag)*TAGS_PER_TIME);
			HANDLE_ERROR(cudaMemcpy(host_tags_buffer,device_tags,sizeof(Tag)*TAGS_PER_TIME,cudaMemcpyDeviceToHost));
			cudaFree(device_string_to_parse);
			cudaFree(device_tags);
			cudaFree(device_tag_info);
			free(host_read_string);
#ifdef CUDA_TIME_TEST
			cudaEventRecord(cuda_parse_finish,0);
			cudaEventSynchronize(cuda_parse_finish);
#endif
			memset(tag_info_bytes,0,sizeof(tag_info)*TAGS_PER_TIME);

			for(int i=0;i< TAGS_PER_TIME;i++) if(host_tags_buffer[i].nameCharIndex>0){

				char* key = host_tags_buffer[i].name;
				hash_map_insert(tags_inverted_index,(void*)key,&host_tags_buffer[i]);
			}

//			free(host_tags_buffer);
			text_ready = 0;
			tag_info_ready = 0;
			idle = 1;
			//notify the master machine that this node has completed the partial parsing work
			MPI_Send(&idle,1,MPI_INT,0,MSG_IDLE,MPI_COMM_WORLD);

#ifdef CUDA_TIME_TEST

			float cuda_parse_time;
			cudaEventElapsedTime(&cuda_parse_time,cuda_parse_start,cuda_parse_finish);
			whole_cuda_parse += cuda_parse_time;
			all_tasks_length += cuda_index;
			cudaEventDestroy(cuda_parse_start);
			cudaEventDestroy(cuda_parse_finish);
#endif
		}
	}
/*
	if(slave_tasks!=0){
		printf("slave average report,(slave_tasks, average_cuda_parse, average_tasks_length)\n"
				"                    (%ld,%.6f,%ld)\n",
				slave_tasks, whole_cuda_parse/slave_tasks, all_tasks_length/slave_tasks);
	}
*/
#ifdef SLAVE_TIME_TEST
	gettimeofday(&slave_stop,NULL);
	double stop_t = slave_stop.tv_sec + slave_stop.tv_usec/1000000.0;
		printf("slave time report:(%.6lf,%.6lf,%.6lf)\n",start_t1,stop_t,stop_t-start_t1);
#endif
return NULL;
}


char processor_name[MPI_MAX_PROCESSOR_NAME];

int main(int argc, char **argv) {
	char *filePath = argv[1];
	int thread_num = atoi(argv[2]);
	strcpy(file_path,filePath);
	task_queue_init();
//
	int numprocs,namelen,rank,devCount, provide_level;
	MPI_Init_thread(&argc,&argv,MPI_THREAD_SINGLE,&provide_level);
	if(provide_level < MPI_THREAD_SINGLE){
		printf("Error: the MPI library doesn't provide the required thread level\n");
		MPI_Abort(MPI_COMM_WORLD,0);
	}
	MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Get_processor_name(processor_name,&namelen);
	printf("Hello from %d on %s out of %d\n", rank, processor_name, numprocs);
	slaves_num = numprocs - 1;
	for(int i=0;i<5;i++)
			idle_node[i] = 1;
	printf("set slaves num %d \n",slaves_num);
	if(rank==0){
//		GDB_WAIT_ATTACH();
		printf("provide level is :%d\n",provide_level);
		slave_parse_stop = 0;
#ifdef HOST_TIME_TEST
		struct timeval host_start, host_stop;
		gettimeofday(&host_start,NULL);
		double start = host_start.tv_sec + host_start.tv_usec/1000000.0;
#endif

		pthread_t thread_schedule;
		pthread_create(&thread_schedule,NULL,schedule,NULL);
		multi_read(filePath,thread_num);
		pthread_join(thread_schedule,NULL);

		printf("-------------------------------------------------\n");
#ifdef HOST_TIME_TEST
		gettimeofday(&host_stop,NULL);
		double stop = host_stop.tv_sec + host_stop.tv_usec/1000000.0;
		printf("host time reprot:(%.6lf,%.6lf,%.6lf)\n",start,stop,stop-start);
		printf("-------------------------------------------------\n");
#endif
	}
	else{
//		GDB_WAIT_ATTACH();
		if(cudaGetDeviceCount(&devCount)!=cudaSuccess){
			 printf("Device error on %s\n", processor_name);
			 MPI_Finalize();
			 return EXIT_FAILURE;
		}

		pthread_t slave_parse_thread;
		pthread_create(&slave_parse_thread,NULL,slave_parse,NULL);
		pthread_join(slave_parse_thread,NULL);

	}

	MPI_Finalize();
	return 0;
}
