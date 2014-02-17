
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
#define MAX_READ_LENGTH 1024*1024*8

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

typedef struct{
	char **host_read_string;
	Tag **host_tags_info;
}cuda_arg;

extern "C"{
	void syn_init();
	void syn_suspend(pthread_cond_t cond);
	void syn_resume(pthread_cond_t cond);
	void* simple_parse(void* arg);
}
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
		done:
		break;
	}//end of read buffer.

}

void* cuda_parse(void* c_arg){

	while(1){
//		printf("cuda_parse start...... \n");
		pthread_cond_wait(&cond_cuda,&syn_mutex);
		printf("cuda receive signal\n");
		simple_parse_arg *arg = (simple_parse_arg*)c_arg;
		char *host_read_string = *(arg->string_read);
		Tag *host_tags_info = *(arg->s_tags_ready);
		char* device_string_to_parse;
		Tag* device_tags_info;

		size_t host_string_len = strlen(host_read_string);
		cudaMalloc((void**)&device_string_to_parse,sizeof(char)*host_string_len);
		cudaMalloc((void**)&device_tags_info,sizeof(Tag)*TAGS_PER_TIME);
		cudaMemcpy(device_string_to_parse,host_read_string,host_string_len,cudaMemcpyHostToDevice);
		cudaMemcpy(device_tags_info,host_tags_info,sizeof(Tag)*TAGS_PER_TIME,cudaMemcpyHostToDevice);
		parse_tag<<<1,1024>>>(device_tags_info, device_string_to_parse);
		cudaMemcpy(host_tags_info,device_tags_info,sizeof(Tag)*TAGS_PER_TIME,cudaMemcpyDeviceToHost);
		cudaFree(device_string_to_parse);
		cudaFree(device_tags_info);
		pthread_cond_signal(&cond_prescan);
		printf("cuda send signal \n");
	}
}





int main(int argc, char **argv) {

//
	pthread_t thread_prescan, thread_cuda_parse;
	pthread_t thread_resume;
//	char filePath[1000]="/home/jerry/Downloads/enwiki-latest-pages-meta-history4.xml-p000104986p000104998";//80.9M
	char *filePath = argv[1];
	Tag* host_tags_info;
	char* host_read_string;
	simple_parse_arg *s_arg;
	s_arg = (simple_parse_arg*)malloc(sizeof(simple_parse_arg));
	s_arg->file_path = filePath;
	s_arg->s_tags_ready = (Tag**)malloc(sizeof(Tag*));
	s_arg->string_read = (char**)malloc(sizeof(char*));
	*(s_arg->s_tags_ready) = (Tag*)malloc(sizeof(Tag));
	*(s_arg->string_read) = (char*)malloc(sizeof(char));

	cuda_arg c_arg;
	clock_t start,stop;
	start = clock();



	pthread_create(&thread_cuda_parse,NULL, cuda_parse,(void*)s_arg);
	pthread_create(&thread_prescan,NULL,simple_parse,(void*)s_arg);
	pthread_join(thread_cuda_parse,NULL);
	pthread_join(thread_prescan,NULL);
//	stop = clock();
//	double dur = (double)(stop - start);
//	printf("\n total time is: %f s\n",dur/CLOCKS_PER_SEC);
	pthread_exit(NULL);

}
