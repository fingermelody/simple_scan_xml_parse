#include <mpi.h>
#include "simple_parser.h"
#include "stack.h"
#include "tag.h"
#include "TagArray.h"
#include "stdafx.h"
#include "Text.h"
#include "task_queue.h"
#include "time.h"
#define MAX_TAGS 100000

void start_tag_handle(tag_info *start_tag, simple_parser* parser){
	parser->start_tag_num ++;
	start_tag->id = parser->start_tag_num + parser->id_base;
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

void parser_init(simple_parser* parser, parser_char_handler char_handler, unsigned long ID_base){
	parser->start_tag_num = 0;
	parser->end_tag_num = 0;
	parser->tasks = 0;
	parser->unresolved_end_tag = 0;
	parser->unresolved_start_tag = 0;
	parser->st = st_Content;
	parser->id_base = ID_base;
	parser->array_tags = (Tag_Array*)malloc(sizeof(Tag_Array));
	tag_array_init(parser->array_tags,DEFAULT_TAG_ARRAY_SIZE);
	parser->un_start_tags = (Tag_Array*)malloc(sizeof(Tag_Array));
	tag_array_init(parser->un_start_tags,UNRESOLVE_TAG_SIZE);
	parser->start_tags = (stackT*)malloc(sizeof(stackT));
	StackInit(parser->start_tags,MAX_TAGS);
	if(char_handler == NULL)
		parser->char_handler = default_parser_char_handler;
}

Tag_Array* parser_merge(simple_parser** parsers, int num){
	int i,j;

	Tag_Array* result = (Tag_Array*)malloc(sizeof(Tag_Array));
	Tag_Array* G_un_starts  = (Tag_Array*)malloc(sizeof(Tag_Array));
	int G_un_start_num = 0;
	if(result == NULL || G_un_starts == NULL){
		printf("merge failed : malloc new Tag_Array failed!\n");
		return NULL;
	}
	tag_array_init(result,TAG_ARRAY_MAX_SIZE);
	tag_array_init(G_un_starts,UNRESOLVE_TAG_SIZE*num);
	tag_array_add_array(result,parsers[0]->array_tags);
	tag_array_add_array(G_un_starts,parsers[0]->un_start_tags);
	G_un_start_num = parsers[0]->unresolved_start_tag;
	for(i=1;i<num;i++){
		for(j=0;j<parsers[i]->start_tag_num;j++){
			if(parsers[i]->array_tags->tags[j].parent < 0){
				int index_of_parent = 0 - parsers[i]->array_tags->tags[j].parent;
				if(index_of_parent != 0){
					parsers[i]->array_tags->tags[j].parent = G_un_starts->tags[index_of_parent].id;
				}else{
					parsers[i]->array_tags->tags[j].parent = 0;
				}
			}
			tag_array_add(result,&(parsers[i]->array_tags->tags[j]));
		}
		G_un_start_num -= parsers[i]->unresolved_end_tag;
		tag_array_pre_delete(G_un_starts,parsers[i]->unresolved_end_tag);
		tag_array_pre_add(G_un_starts,parsers[i]->un_start_tags);
		G_un_start_num += parsers[i]->unresolved_start_tag;
	}

	return result;
}
