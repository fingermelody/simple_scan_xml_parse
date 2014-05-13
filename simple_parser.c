#include <mpi.h>
#include "simple_parser.h"
#include "stack.h"
#include "tag.h"
#include "TagArray.h"
#include "stdafx.h"
#include "Text.h"
#include "task_queue.h"
#include "time.h"
#define MAX_TAGS 1000000

void start_tag_handle(tag_info *start_tag, simple_parser* parser){
	parser->start_tag_num ++;
	start_tag->id = parser->start_tag_num;
	StackPush(parser->start_tags,start_tag);
	parser->unresolved_start_tag ++;
}

void end_tag_handle(tag_info *end_tag, simple_parser* parser){

	if(parser->unresolved_start_tag <= 0){
		parser->unresolved_end_tag++;
		return;
	}
	parser->unresolved_start_tag--;
	tag_info *s_tag = (tag_info*)StackPop(parser->start_tags);
	tag_info* p = (tag_info*)StackGetTop(parser->start_tags);
	if (p == NULL)
		s_tag->parent = 0- parser->unresolved_end_tag;
	else
		s_tag->parent = p->id;
	tag_array_add(parser->array_tags,s_tag);
	free(s_tag);

	parser->end_tag_num++;
	if(parser->end_tag_num%TAGS_PER_TIME == 0){
		parse_task *task = (parse_task*)malloc(sizeof(parse_task));
		task->info = &(parser->array_tags->tags[parser->tasks*TAGS_PER_TIME]);
		task->tag_num = TAGS_PER_TIME;
		add_task(task);
		parser->tasks++;
	}

}

/*
 * analysis the basic structure information of tags while reading the whole xml file.
 * If we read enough tags, notify the schedule thread to transfer the tags to idle slaves.
 * After transferring this part of file, continue reading work.
 * */
void default_parser_char_handler(void* v_parser,char cur,long loc){
	simple_parser* parser = (simple_parser*)v_parser;

	switch (parser->st)
	{
	case st_Content:
		if (cur == '<')
		{
			parser->st = st_LT;
		}
		break;
	case st_LT:
		if (cur == '/')
		{
			parser->st = st_End_Tag;
		}
		else if (cur == '?' || cur == '!')
		{
			parser->st = st_PI_Comment_CDATA;
		}
		else
		{
			parser->tag_tmp = (tag_info*)malloc(sizeof(tag_info));
			tag_info_init(parser->tag_tmp);
			parser->tag_tmp->location = loc-1;
			parser->st = st_Start_Tag;
		}
		break;
	case st_End_Tag:
		if (cur == '>')
		{
			parser->st = st_Content;
			end_tag_handle(NULL,parser);
		}
		break;
	case st_PI_Comment_CDATA:
		if (cur == '>')
		{
			parser->st = st_Content;
		}else if (cur == '/')
		{
			parser->st = st_Empty_Tag;
		}
		break;
	case st_Start_Tag:
		if (cur == '"' || cur == '\'')
		{
			parser->st = st_Alt_Val;
		}else if(cur == '/'){
			parser->st = st_Whole_tag_pre;
		}else if (cur == '>')
		{
			parser->st = st_Content;
			parser->tag_tmp->length = loc - parser->tag_tmp->location + 1;
			start_tag_handle(parser->tag_tmp,parser);
		}
		break;
	case st_Alt_Val:
		if (cur == '"' || cur == '\'')
		{
			parser->st = st_Start_Tag;
		}
		break;
	case st_Empty_Tag:
		if (cur == '>')
		{
			parser->st = st_Content;
		}
		break;
	case st_Whole_tag_pre:
		if(cur == '>'){
			parser->st = st_Content;
			parser->tag_tmp->length = loc - parser->tag_tmp->location + 1 ;
			start_tag_handle(parser->tag_tmp,parser);
			end_tag_handle(NULL, parser);
		}else{
			parser->st = st_Start_Tag;
		}
		break;

	default:
		break;
	}
}

void parser_init(simple_parser* parser, parser_char_handler char_handler){
	parser->start_tag_num = 0;
	parser->end_tag_num = 0;
	parser->tasks = 0;
	parser->unresolved_end_tag = 0;
	parser->unresolved_start_tag = 0;
	parser->st = st_Content;
	parser->array_tags = (Tag_Array*)malloc(sizeof(Tag_Array));
	tag_array_init(parser->array_tags,DEFAULT_TAG_ARRAY_SIZE);
	parser->start_tags = (stackT*)malloc(sizeof(stackT));
	StackInit(parser->start_tags,MAX_TAGS);
	if(char_handler == NULL)
		parser->char_handler = default_parser_char_handler;
}

Tag_Array* parser_merge(simple_parser** parsers, int num){
	int i,j;
	int all_tag_count=0;
	for(i=0;i<num;i++){
		all_tag_count += parsers[i]->start_tag_num;
	}
	Tag_Array* result = (Tag_Array*)malloc(sizeof(Tag_Array));
	if(result == NULL){
		printf("merge failed : malloc new Tag_Array failed");
		return NULL;
	}
	tag_array_init(result,all_tag_count);
	tag_array_add_array(result,parsers[0]->array_tags);
	int sum_pre_id = parsers[0]->start_tag_num;
	for(i=1;i<num;i++){
		for(j=0;j<parsers[i]->start_tag_num;j++){
			parsers[i]->array_tags->tags[j].id += sum_pre_id;
			if(parsers[i]->array_tags->tags[j].parent < 0){
				int backward_index_of_parent = parsers[i-1]->unresolved_start_tag + parsers[i]->array_tags->tags[j].parent;
				if(backward_index_of_parent != 0){
					int pre_end = parsers[i-1]->start_tag_num;
					parsers[i]->array_tags->tags[j].parent = parsers[i-1]->array_tags->tags[pre_end-backward_index_of_parent].id;
				}else{
					parsers[i]->array_tags->tags[j].parent = 0;
				}
			}
			tag_array_add(result,&(parsers[i]->array_tags->tags[j]));
		}
		sum_pre_id += parsers[i]->start_tag_num;
	}

	return result;
}
