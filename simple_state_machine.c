#include "simple_state_machine.h"
#include "stack.h"
#include "tag.h"
#include "node.h"
#include "TagArray.h"
#include "stdafx.h"
#include <pthread.h>
#define MAX_TAGS 1000000
#define MAX_NODES 1000000
#define CUR pBuffer[curIndex]
#define ISEND (curIndex >= length)
#define MAX_READ_LENGTH 1024*1024*8


stackT start_tags;
stackT nodes;
Tag_Array array_tags;
Tag *tag_tmp;
int node_num = 0;
int round = 0;
void start_tag_handle(Tag *start_tag){
	StackPush(&start_tags,(stackElementT)start_tag);
	StackGetTop(&start_tags);
}

void end_tag_handle(Tag *end_tag){
	Tag *s_tag = (Tag*)StackPop(&start_tags);	
	s_tag->parent = ((Tag*)StackGetTop(&start_tags))->id;
	tag_array_add(array_tags,s_tag);
	node_num++;
}

int generate_tag_id(int round, int current){
	return round*TAGS_PER_TIME+current;
}
/**
 **this function collect the location and length of each tag ready to parse
**/
void get_tag_info_of_ready_node(stackT *stackP,Tag* tags){
	tags = (Tag*)malloc(sizeof(Tag)*TAGS_PER_TIME);
	for(int i=0;i<=stackP->top;i++)
		tags[i] = *(Tag*)(stackP->contents[i]);
	free(tags);
}

void simple_parse(_IN FILE *file, _IN char* pBuffer, _OUT Tag *s_tags_ready,_OUT char *string_read){
	
	StackInit(&start_tags, MAX_TAGS);
	StackInit(&nodes, MAX_NODES);
	pBuffer = (char*)malloc(sizeof(char)*MAX_READ_LENGTH);
	int res;
	simple_state st;
	st = st_Content;
	while (res = fread(pBuffer,1,MAX_READ_LENGTH,file))
	{
	
		int counter = 0;//counter indicates the number of start_tags
		int length = res;
		int curIndex = 0;
		while (!ISEND)
		{
			char cur = CUR;
		
			switch (st)
			{
			case st_Content:
				if (cur == '<')
				{
					st = st_LT;
					tag_init(tag_tmp);
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
					end_tag_handle(tag_tmp);
					//when we read TAGS_PER_TIME nodes, we will transfer them to the nodes pool
					if(counter%TAGS_PER_TIME == 0){
						s_tags_ready = &(array_tags.tags[round*TAGS_PER_TIME]);
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
					tag_tmp->id = generate_tag_id(round,counter);
					tag_tmp->lengh = curIndex - tag_tmp->location + 1;
					start_tag_handle(tag_tmp);
					counter++;
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

			curIndex ++;
		}
	}
}
