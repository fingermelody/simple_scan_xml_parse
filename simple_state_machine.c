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
#include "task_queue.h"
#include "time.h"
#define MAX_TAGS 1000000
#define MAX_NODES 1000000

stackT start_tags;
stackT nodes;
Tag_Array *array_tags;
tag_info *tag_tmp;
int node_num = 0;
int task_round_read = 0;
void start_tag_handle(tag_info *start_tag){
	StackPush(&start_tags,start_tag);
}

void end_tag_handle(){
	tag_info *s_tag = (tag_info*)StackPop(&start_tags);
	tag_info* p = (tag_info*)StackGetTop(&start_tags);
	if (p == NULL)
		s_tag->parent = -1;
	else
		s_tag->parent = p->id;
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
#ifdef L_DEBUG
	printf("tags_stack ready\n");
#endif
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
	tag_tmp = (tag_info*)malloc(sizeof(tag_info));
	char cur;
#ifdef TIME_TEST
	read_start = clock();
#endif
	while (1){
		cur=fgetc(file);
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
						tag_tmp = (tag_info*)malloc(sizeof(tag_info));
						tag_info_init(tag_tmp);
						tag_tmp->location = ftell(file)-2;
						st = st_Start_Tag;
					}
					break;
				case st_End_Tag:
					if (cur == '>')
					{
						st = st_Content;
						end_tag_handle();
						counter++;
						if(counter%TAGS_PER_TIME == 0){
							//produce parse_task and add it into task queue
							parse_task *task = (parse_task*)malloc(sizeof(parse_task));
							task->info = &(array_tags->tags[task_round_read*TAGS_PER_TIME]);
							task->tag_num = TAGS_PER_TIME;
							add_task(task);
							task_round_read++;
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
					}else if(cur == '/'){
						st = st_Whole_tag_pre;
					}else if (cur == '>')
					{
						st = st_Content;
						tag_tmp->length = ftell(file) - tag_tmp->location;
						tag_tmp->id = start_tag_num;
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
				case st_Whole_tag_pre:
					if(cur == '>'){
						st = st_Content;
						tag_tmp->id = start_tag_num;
						tag_tmp->length = ftell(file) - tag_tmp->location;
						start_tag_handle(tag_tmp);
						start_tag_num++;

						end_tag_handle();
						counter++;

						if(counter%TAGS_PER_TIME == 0){
							//produce parse_task and add it into task queue
							parse_task *task = (parse_task*)malloc(sizeof(parse_task));
							task->info = &(array_tags->tags[task_round_read*TAGS_PER_TIME]);
							task->tag_num = TAGS_PER_TIME;
							add_task(task);
							task_round_read++;
						}
					}else{
						st = st_Start_Tag;
					}
					break;

				default:
					break;
				}
			}
	//if we have read all of the file but there are not enough 1024 tags, we have to transfer what we've read to GPU parser.
	if(counter%TAGS_PER_TIME !=0 ){
			parse_task *task = (parse_task*)malloc(sizeof(parse_task));
			task->info = &(array_tags->tags[task_round_read*TAGS_PER_TIME]);
			task->tag_num = counter%TAGS_PER_TIME;
			add_task(task);
			task_round_read++;
		}
		file_read_over = 1;

		fclose(file);

#ifdef TIME_TEST
		task_counter = counter/TAGS_PER_TIME + 1;
		read_finish = clock();
#endif
	return NULL;
}
