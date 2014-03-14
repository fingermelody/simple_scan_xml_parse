#include "synchronize.h"
#include "simple_state_machine.h"
#include "stack.h"
#include "tag.h"
#include "node.h"
#include "stdafx.h"
#include "cuda_runtime.h"
#include "cuda.h"
#include "device_launch_parameters.h"
#include <string.h>
#include "simple_scan_parser.h"
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <mpi.h>
#include "Text.h"
#include "tag.h"
#include "tools/hash_map.h"
#include "MPQ_hash.h"
#include "slaves.h"
#include "MPI_MSG_TYPE.h"
#include "debug.h"

#define TagNameAddChar(tag, c) do{			\
	tag->name[tag->nameCharIndex] = c;		\
	tag->nameCharIndex++;					\
}while(0)

#define TagAddAttr(tag, attr) do{			\
	tag->attributes[tag->attrIndex]=attr;	\
	tag->attrIndex++;						\
}while(0)

#define AttrNameAddChar(attr, c) do{		\
	attr->name[attr->nameCharIndex] = c;    \
	attr->nameCharIndex++;					\
}while(0)

#define AttrValueAddChar(attr,c) do{		\
	attr->value[attr->valueCharIndex] = c;  \
	attr->valueCharIndex++;					\
}while(0)

#define HANDLE_ERROR( err ) (HandleError( err, __FILE__, __LIN-E__ ))

//__constant__ unsigned char pBuffer[16*16*16*16*LENGTH];

//static void HandleError( cudaError_t err,
//                         const char *file,
//                         int line ) {
//    if (err != cudaSuccess) {
//        printf( "%s in %s at line %d\n", cudaGetErrorString( err ),
//                file, line );
//        exit( EXIT_FAILURE );
//    }
//}

typedef struct{
	char **host_read_string;
	Tag **host_tags_info;
}cuda_arg;

extern "C"{
	void syn_init();
	void syn_suspend(pthread_cond_t cond);
	void syn_resume(pthread_cond_t cond);
	void* simple_parse(void* arg);
	void hash_map_init(hash_map *map, size_t capacity, hash_map_comparator comparator, hash_map_hash_func hash_func);
	void hash_map_insert(hash_map *map, void *key, void *value);
	void prepareCryptTable();
	size_t hash_map_MPQ_hash_func(const void *key, size_t capacity);
	int MPQ_comparator(const void* l, const void *r);
	void* slave_query_worker(void* argc);
	void* query(void* argc);
	void* schedule(void* argc);
	void* monitor(void* argc);
}

/*
 * GPU kernel. parse the xml file to tags' detail
 * @param: tags_info, tags' basic structure information, this rountine will complete the information in this Tag list.
 * @param: chars of partial xml file got from master.
 * */
__global__ void parse_tag(Tag* tags_info, char* string){
	int id = threadIdx.x;
	Tag t = tags_info[id];
	int s_location = t.location;
	int len = t.lengh;
	
	int curIndex = s_location;
	enum _state st = st_idle;
	attribute tmp_attr;
	while(curIndex - s_location < len){
		char cur = string[curIndex];

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
				st = st_start_tag;
				break;
			}//end of state tagstart
			case st_start_tag:{//record tag name here
				if(' ' == cur){
					memset(&tmp_attr,0,sizeof(attribute));
					st = st_attribute_pre;
				}else if('>' == cur){
					st = st_idle;
				}else{// else this char belongs to tag name
					TagNameAddChar((&t),cur);
				}
				break;
			}//end of state st_start_tag
			case st_attribute_pre:{
				if(' ' == cur){
					//ignore space
				}else if('>' == cur){// normal end
					st = st_idle;
				}else{
					AttrNameAddChar((&t),cur);
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
					AttrNameAddChar((&t),cur);
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
					TagAddAttr((&t),tmp_attr);
					//free(tmp_attr);
					st = st_attribute_pre;
				}else{
					AttrValueAddChar((&tmp_attr),cur);
				}
				break;
			}

		}//end of switch
		
		curIndex ++;
		continue;
	}//end of read buffer.

}


/*
 * slave thread will receive the partial XML file from master, and parse the xml into tags details,
 * including tag name, attribute,then uses these details to build tag invert index, which includes
 * the tag name as the key and the tag id as the value.
 * */
void* slave_parse(void* argc){

	MPI_Status status;
	size_t inverted_index_map_size = 134217728; //2^27
	tags_inverted_index = (hash_map*)malloc(sizeof(hash_map));
	hash_map_init(tags_inverted_index,inverted_index_map_size,MPQ_comparator,hash_map_MPQ_hash_func);
	prepareCryptTable();//CryptTable is for the MPQ hash algorithm
	char* host_read_string; //buffer for receiving the chars of the partial xml file
	host_read_string = (char*)malloc(sizeof(char)*MAX_TEXT_LENGTH);
	memset(host_read_string,0,sizeof(char)*MAX_TEXT_LENGTH);
	char* tag_info_bytes; //buffer for receiving Tags' information in the partial xml file
	tag_info_bytes =(char*)malloc(TAGS_PER_TIME*sizeof(Tag));
	memset(tag_info_bytes,0,sizeof(Tag)*TAGS_PER_TIME);
	int slave_rank;
	MPI_Comm_rank(MPI_COMM_WORLD,&slave_rank);
	printf("%d slave thread start \n",slave_rank);
	Tag* host_tags_buffer = (Tag*)malloc(sizeof(Tag)*TAGS_PER_TIME);//buffer for restoring the tag parse result.
	memset(host_tags_buffer,0,sizeof(Tag)*TAGS_PER_TIME);
	while(1){
//		MPI_Probe(0,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
//		printf("slave receive a message %d\n",status.MPI_TAG);
		int test = 0;
		MPI_Recv(&test,1,MPI_INT,0,MSG_TEST,MPI_COMM_WORLD,&status);
		printf("test is %d \n",test);
		//receive the partial xml file and the structure informations of tags in this part of file
//		MPI_Recv(host_read_string,10240000,MPI_CHAR,0,MSG_SEND_STRING,MPI_COMM_WORLD,&status);
//		MPI_Recv(tag_info_bytes,TAGS_PER_TIME*sizeof(Tag),MPI_CHAR,0,MSG_SEND_TAG_INFO,MPI_COMM_WORLD,&status);
		printf("slave %d receive successfully\n, %s",slave_rank,host_read_string);
//		Tag *host_tags_info = (Tag*)tag_info_bytes;
//		memset(host_tags_buffer,0,TAGS_PER_TIME*sizeof(Tag));
//
//		char* device_string_to_parse;
//		Tag* device_tags_info;
//		size_t host_string_len = strlen(host_read_string);
//
//		cudaMalloc((void**)&device_string_to_parse,sizeof(char)*host_string_len);
//		cudaMalloc((void**)&device_tags_info,sizeof(Tag)*TAGS_PER_TIME);
//
//		cudaMemcpy(device_string_to_parse,host_read_string,host_string_len,cudaMemcpyHostToDevice);
//		cudaMemcpy(device_tags_info,host_tags_info,sizeof(Tag)*TAGS_PER_TIME,cudaMemcpyHostToDevice);
//
//		parse_tag<<<1,1024>>>(device_tags_info, device_string_to_parse);
//
//		cudaMemcpy(&host_tags_buffer,device_tags_info,sizeof(Tag)*TAGS_PER_TIME,cudaMemcpyDeviceToHost);
//		cudaFree(device_string_to_parse);
//		cudaFree(device_tags_info);
//		idle = 1;
//		for(int i=0;(i<TAGS_PER_TIME)&&(host_tags_buffer[i].nameCharIndex>0);i++){
//			char* key = host_tags_buffer[i].name;
//			int id = host_tags_buffer[i].id;
//			hash_map_insert(tags_inverted_index,(void*)key,(void*)(&id));
//		}
//		//
//		MPI_Probe(0,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
//		if(status.MPI_TAG == MSG_EXIT)
//			break;
	}
//	pthread_t slave_query_thread;
////	int* temp_arg;
//	pthread_create(&slave_query_thread,NULL,slave_query_worker,NULL);
//	pthread_join(slave_query_thread,NULL);
//	return NULL;


}


char processor_name[MPI_MAX_PROCESSOR_NAME];

int main(int argc, char **argv) {
	char *filePath = argv[1];
	simple_parse_arg *s_arg;

	int numprocs,namelen,rank,devCount;
	MPI_Init(&argc,&argv);
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
		//initial values
		file_read_over = 0;
		slave_parse_stop = 0;
		s_arg = (simple_parse_arg*)malloc(sizeof(simple_parse_arg));
		s_arg->file_path = filePath;
		s_arg->s_tags_ready = (Tag**)malloc(sizeof(Tag*));
		s_arg->string_read = (char**)malloc(sizeof(char*));
		*(s_arg->s_tags_ready) = (Tag*)malloc(sizeof(Tag));
		*(s_arg->string_read) = (char*)malloc(sizeof(char));
		pthread_t thread_prescan, thread_schedule, thread_monitor;
		pthread_create(&thread_prescan,NULL,simple_parse,(void*)s_arg);
		pthread_create(&thread_schedule,NULL,schedule,(void*)s_arg);
		pthread_create(&thread_monitor,NULL,monitor,NULL);
		pthread_join(thread_prescan,NULL);
		pthread_join(thread_schedule,NULL);
	}
	else{
		if(cudaGetDeviceCount(&devCount)!=cudaSuccess){
			 printf("Device error on %s\n", processor_name);
			 MPI_Finalize();
			 return EXIT_FAILURE;
		}

//		GDB_WAIT_ATTACH();
//		int* temp_arg;
		pthread_t slave_parse_thread;
		pthread_create(&slave_parse_thread,NULL,slave_parse,NULL);
		pthread_join(slave_parse_thread,NULL);


	}

	MPI_Finalize();
	return 0;
}
