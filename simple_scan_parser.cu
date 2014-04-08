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
	void* simple_parse(void* arg);
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
}

__global__ void gpu_test(tag_info* infos,char* string, int* ids,Tag* tags){
	int id = threadIdx.x;
	ids[id] = infos[id].id;
	Tag* t = (&tags[id]);

}

/*
 * GPU kernel. parse the xml file to tags' detail
 * @param: tags_info, tags' basic structure information, this rountine will complete the information in this Tag list.
 * @param: chars of partial xml file got from master.
 * */
__global__ void parse_tag(tag_info* tags_info, char* s, int base, Tag* tags){

	int id = threadIdx.x;
	tag_info info = tags_info[id];

	Tag* t = &(tags[id]);
	int s_location = info.location;
	int len = info.lengh;
	if(len == 0) return;

	t->info = info;



	int curIndex = s_location;
	int end = curIndex + len;
	if(s[curIndex]=='<')
		t->name[0] = 'R';
	else
		t->name[0] = 'E';
//	TagNameAddChar(t,string[curIndex]);

	enum _state st = st_idle;
	attribute tmp_attr;



	while(curIndex  < end){
		char cur = s[curIndex];
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

	MPI_Status s_prob,s_recv_txt,s_recv_info;
	MPI_Request request;
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

	int base = 0;
	int slave_rank;
	int text_ready = 0, tag_info_ready = 0;
	int tags_num;
	MPI_Comm_rank(MPI_COMM_WORLD,&slave_rank);

	Text* t;
	tag_info* host_tag_infos;
	char* host_read_string;
	int l,r,ii=0;
	//receive the partial xml file and the structure informations of tags in this part of file
	while(1){
		MPI_Probe(0,MPI_ANY_TAG,MPI_COMM_WORLD,&s_prob);
		printf("slave receive a message %d\n",s_prob.MPI_TAG);
		int count;
		MPI_Get_count(&s_prob,MPI_CHAR,&count);
		switch(s_prob.MPI_TAG){
		case MSG_TEXT:
			MPI_Recv(bytes_of_text,count,MPI_CHAR,0,MSG_TEXT,MPI_COMM_WORLD,&s_recv_txt);
			t = (Text*)bytes_of_text;
			base = t->offset;
			printf("received text length is %d,offset is %d \n",t->length,t->offset);
			host_read_string = (char*)malloc(t->length);

		    l = lseek(pf,t->offset,SEEK_SET);
			if(l == -1) printf("lseek failed 000000000000000000000000000000\n");
			r = read(pf,host_read_string,t->length);
			if(r==-1) printf("read text failed 99999999999999999999999999999\n");
			else printf("receive text successfully, l:%d,r:%d\n",l,r);
			printf("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
			printf("first 10 char of read text:%c,%c,%c,%c,%c,%c,%c,%c,%c,%c!!!\n",host_read_string[0],host_read_string[1],
					host_read_string[2],host_read_string[3],host_read_string[4],host_read_string[5],host_read_string[6],
					host_read_string[7],host_read_string[8],host_read_string[9]);
//			if(ii == 1) printf("%s\n",host_read_string);
			text_ready = 1;
//			printf("%s\n",host_read_string);

			break;
		case MSG_SEND_TAG_INFO:
			MPI_Recv(tag_info_bytes,count,MPI_CHAR,0,MSG_SEND_TAG_INFO,MPI_COMM_WORLD,&s_recv_info);
			tag_info_ready = 1;
			tags_num = count/sizeof(tag_info);
			host_tag_infos = (tag_info*)tag_info_bytes;
			printf("receive tag info successfully\n");
			printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
			printf("slave tag 20's info is following:\n");
			printf("id:%d length:%d location:%d\n",host_tag_infos[20].id,host_tag_infos[20].lengh,host_tag_infos[20].location);
			break;
		case MSG_EXIT:
			int end;
//			MPI_Recv(&end,0,MPI_INT,0,MSG_EXIT,MPI_COMM_WORLD,&status);
			printf("EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n");
//			pthread_exit(NULL);
			return NULL;
//
		}


		if(text_ready && tag_info_ready){


			char* device_string_to_parse;
			Tag* device_tags;
			tag_info* device_tag_info;
			HANDLE_ERROR(cudaMalloc((void**)&device_string_to_parse,t->length));
			HANDLE_ERROR(cudaMemset(device_string_to_parse,0,t->length));
			HANDLE_ERROR(cudaMalloc((void**)&device_tags,sizeof(Tag)*TAGS_PER_TIME));
			HANDLE_ERROR(cudaMemset(device_tags,0,sizeof(Tag)*TAGS_PER_TIME));
			HANDLE_ERROR(cudaMalloc((void**)&device_tag_info,sizeof(tag_info)*TAGS_PER_TIME));
			HANDLE_ERROR(cudaMemset(device_tag_info,0,sizeof(tag_info)*TAGS_PER_TIME));
			HANDLE_ERROR(cudaMemcpy(device_string_to_parse,host_read_string,t->length,cudaMemcpyHostToDevice));
			HANDLE_ERROR(cudaMemcpy(device_tag_info,host_tag_infos,sizeof(tag_info)*TAGS_PER_TIME,cudaMemcpyHostToDevice));


//			printf("tags num is %d\n",tags_num);
			parse_tag<<<1,tags_num>>>(device_tag_info, device_string_to_parse,base,device_tags);

			Tag* host_tags_buffer = (Tag*)malloc(sizeof(Tag)*TAGS_PER_TIME);//buffer for restoring the tag parse result.
			memset(host_tags_buffer,0,sizeof(Tag)*TAGS_PER_TIME);
			HANDLE_ERROR(cudaMemcpy(host_tags_buffer,device_tags,sizeof(Tag)*TAGS_PER_TIME,cudaMemcpyDeviceToHost));
			cudaFree(device_string_to_parse);
			cudaFree(device_tags);
			cudaFree(device_tag_info);
			free(host_read_string);

			memset(tag_info_bytes,0,sizeof(tag_info)*TAGS_PER_TIME);
			printf("**********************************************************\n");
			printf("name: %s,id: %d\n",host_tags_buffer[20].name,host_tags_buffer[20].info.id);
			for(int i=0;i< TAGS_PER_TIME;i++) if(host_tags_buffer[i].nameCharIndex>0){

				char* key = host_tags_buffer[i].name;
//				printf("name :%s\n",key);
				int id = host_tags_buffer[i].info.id;
				hash_map_insert(tags_inverted_index,(void*)key,(void*)(&id));
			}
			free(host_tags_buffer);
			text_ready = 0;
			tag_info_ready = 0;
			idle = 1;
			//notify the master machine that this node has completed the partial parsing work
			MPI_Send(&idle,1,MPI_INT,0,MSG_IDLE,MPI_COMM_WORLD);
//			MPI_Wait(&request,&status);

			printf("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\n");
		}
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
	strcpy(file_path,filePath);
	simple_parse_arg *s_arg;
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
		//initial values
		printf("provide level is :%d\n",provide_level);
		file_read_over = 0;
		slave_parse_stop = 0;
		s_arg = (simple_parse_arg*)malloc(sizeof(simple_parse_arg));
		s_arg->file_path = filePath;
		s_arg->s_tags_ready = (tag_info**)malloc(sizeof(tag_info*));
		s_arg->text_read = (Text**)malloc(sizeof(Text*));
		*(s_arg->s_tags_ready) = (tag_info*)malloc(sizeof(tag_info));
		*(s_arg->text_read) = (Text*)malloc(sizeof(Text));
		pthread_t thread_prescan, thread_schedule;
		pthread_create(&thread_prescan,NULL,simple_parse,(void*)s_arg);
		pthread_create(&thread_schedule,NULL,schedule,(void*)s_arg);
		pthread_join(thread_prescan,NULL);
		pthread_join(thread_schedule,NULL);
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
