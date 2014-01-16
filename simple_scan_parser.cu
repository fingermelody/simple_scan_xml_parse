#include "stack.h"
#include "tag.h"
#include "node.h"
#include "simple_state_machine.h"
#include "stdafx.h"
#include "cuda_runtime.h"
#include "cuda.h"
#include "device_launch_parameters.h"
#include <string.h>
#include "simple_scan_parser.h"
#include "synchronize.h"
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

__global__ void parse_tag(Tag* tags_info, char* string){
	int id = threadIdx.x;
	Tag t = tags_info[id];
	int s_location = t.location;
	int len = t.lengh;
	
	int curIndex = s_location;
	int start = 0;
	int lastLTIndex = 0;
	int firstGTIndex = 0;
	enum _state st = st_idle;
	attribute *tmp_attr;
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
					tmp_attr = (attribute*)malloc(sizeof(attribute));
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
					TagAddAttr((&t),*tmp_attr);
					free(tmp_attr);
					st = st_attribute_pre;
				}else{
					AttrValueAddChar(tmp_attr,cur);
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


void cuda_parse(char *host_read_string, Tag *host_tags_info){
	while(1){
		syn_suspend(cond_cuda);
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
		syn_resume(&cond_pre);
	}
}


int main(int argc, char **argv) {

	syn_init();

	pthread_t thread_prescan, thread_cuda_parse;

	char filePath[1000]="/home/jerry/Downloads/enwiki-latest-pages-meta-history4.xml-p000104986p000104998";//80.9M
	FILE *file;
	file = fopen(filePath,"r");
	if(!file)
	{
		printf("open file failed \n");
		return -1;
	}
	char* host_buffer;
	Tag* host_tags_info;
	char* host_read_string;
	simple_parse(file,host_buffer,host_tags_info,host_read_string);
	

	pthread_create(&thread_prescan,NULL,simple_parse(file,host_buffer,host_tags_info,host_read_string),(void*)&argc);
	pthread_create(&thread_cuda_parse,NULL,cuda_parse(host_read_string,host_tags_info),(void*)&argc);

	pthread_join(thread_prescan);
	pthread_join(thread_cuda_parse);

}
