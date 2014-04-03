#include <mpi.h>
#include "simple_state_machine.h"
#include "stack.h"
#include "tag.h"
#include "node.h"
#include "TagArray.h"
#include "stdafx.h"
#include "Text.h"
#include "synchronize.h"
#include "query.h"

#define MAX_TAGS 1000000
#define MAX_NODES 1000000

stackT start_tags;
stackT nodes;
Tag_Array *array_tags;
tag_info *tag_tmp;
int node_num = 0;
int round_read = 0;
void start_tag_handle(tag_info *start_tag){
	StackPush(&start_tags,start_tag);
}

void end_tag_handle(int id){
	tag_info *s_tag = (tag_info*)StackPop(&start_tags);
	s_tag->id = id;
//	tag_info *i = (tag_info*)(start_tags.contents[0]);
	s_tag->parent = ((tag_info*)StackGetTop(&start_tags))->id;
	tag_array_add(array_tags,s_tag);
}

int generate_tag_id(int round_read, int current){
//	return round_read*TAGS_PER_TIME+current;
	return current;
}
/**
 **this function collect the location and length of each tag ready to parse
**/
void get_tag_info_of_ready_node(stackT *stackP,Tag* tags){
	tags = (Tag*)malloc(sizeof(Tag)*TAGS_PER_TIME);
	int i;
	for(i=0;i<=stackP->top;i++)
		tags[i] = *(Tag*)(stackP->contents[i]);
	free(tags);
}

/*
 * analysis the basic structure information of tags while reading the whole xml file.
 * If we read enough tags, notify the schedule thread to transfer the tags to idle slaves.
 * After transferring this part of file, continue reading work.
 * */
void* simple_parse(void* arg){
//	printf("pre_scan starts......\n");
	simple_parse_arg *s_arg = (simple_parse_arg*) arg;
	_IN char* file_path = s_arg->file_path;
	StackInit(&start_tags, MAX_TAGS);
	array_tags = (Tag_Array*)malloc(sizeof(Tag_Array));
	tag_array_init(array_tags);
	printf("tags_stack ready\n");
	enum simple_state st;
	st = st_Content;
	Text *text;
	text = (Text*)malloc(sizeof(Text));
	text_init(text);
	int text_pre_pos;

	//begin to scan the file
	FILE *file = fopen(file_path,"r");
	if(!file)
	{
		printf("open file failed \n");
		return NULL;
	}
	text_pre_pos = ftell(file);
	int counter = 0;//counter indicates the number of whole node
	int start_tag_num = 0;
	int curIndex = -1;
	tag_tmp = (tag_info*)malloc(sizeof(tag_info));
	char cur;
	while (1){
		cur=fgetc(file);
		curIndex ++;
		if(cur == EOF){
			printf("end of file \n");
			break;
		}
		if(cur=='\0') {
			printf("read error!!\n");
			break;
		}
		switch (st)
			{
			case st_Content:
				if (cur == '<')
				{
					st = st_LT;
					tag_tmp = (tag_info*)malloc(sizeof(tag_info));
					tag_info_init(tag_tmp);
					tag_tmp->location = curIndex;
				}
				break;
			case st_LT:
				if (cur == '/')
				{
					st = st_End_Tag;
				}
				else if (cur == '?' || cur == '!')
				{
					st = st_PI_Comment_CDATA;
				}
				else
				{
					st = st_Start_Tag;
				}
				break;
			case st_End_Tag:
				if (cur == '>')
				{
					st = st_Content;
					end_tag_handle(counter);
					counter++;
					//when we read TAGS_PER_TIME nodes, we will transfer them to the nodes pool
					//and pause the read work
					//when we resume the read work, we reset the curIndex and Text
					if((counter)%TAGS_PER_TIME == 0){
						printf("start_tag:%d, counter:%d,\n",start_tag_num,counter);
						*(s_arg->s_tags_ready) = &(array_tags->tags[round_read*TAGS_PER_TIME]);
						int text_cur_pos = ftell(file);
						text_set(text,text_pre_pos,text_cur_pos);
						*(s_arg->text_read) = text;
						//resume the parse thread
						pthread_cond_signal(&cond_schedule);
						printf("scanner signal send\n");
						//pause read thread here
						pthread_cond_wait(&cond_prescan,&syn_mutex);
						printf("prescan receive signal \n");
						//reset the curIndex and text
						curIndex = -1;
						round_read++;
						text_pre_pos = text_cur_pos;
						text_init(text);
					}
				}
				break;
			case st_PI_Comment_CDATA:
				if (cur == '>')
				{
					st = st_Content;
				}else if (cur == '/')
				{
					st = st_Empty_Tag;
				}
				break;
			case st_Start_Tag:
				if (cur == '"' || cur == '\'')
				{
					st = st_Alt_Val;
				}else if (cur == '>')
				{
					st = st_Content;
//					printf("id: %d\n",tag_tmp->id);
					tag_tmp->lengh = curIndex - tag_tmp->location + 1;
//					printf("length: %d\n",tag_tmp->lengh);
					start_tag_handle(tag_tmp);
					start_tag_num++;
				}
				break;
			case st_Alt_Val:
				if (cur == '"' || cur == '\'')
				{
					st = st_Start_Tag;
				}
				break;
			case st_Empty_Tag:
				if (cur == '>')
				{
					st = st_Content;
				}
				break;
			default:
				break;
			}

	}
	//if we have read all of the file but there are not enough 1024 tags, we have to transfer what we've read to GPU parser.

		*(s_arg->s_tags_ready) = &(array_tags->tags[round_read*TAGS_PER_TIME]);
		int text_cur_pos = ftell(file);
		text_set(text,text_pre_pos,text_cur_pos);
		*(s_arg->text_read) = text;
	//resume the schedule thread
		file_read_over = 1;
		pthread_cond_signal(&cond_schedule);
		printf("prescan send signal \n");
		printf("the whole file's scan work completed \n");
		fclose(file);
		pthread_cond_wait(&cond_prescan,&syn_mutex);
//		pthread_exit(NULL);
		//then we can start to query
//		pthread_t thread_master_query;
//		int* temp_arg;
//		pthread_create(&thread_master_query,NULL,query,(void*)temp_arg);

	return NULL;
}
