#ifndef SIMPLE_STATE_MACHINE_H
#define SIMPLE_STATE_MACHINE_H

#include "stdafx.h"
#include "multi_read.h"
#include "stack.h"
#include "TagArray.h"
#define TAGS_PER_TIME 1024
enum simple_state{
	st_Content,
	st_LT,
	st_End_Tag,
	st_PI_Comment_CDATA,
	st_Start_Tag,
	st_Alt_Val,
	st_Empty_Tag,
	st_Whole_Tag,
	st_Whole_tag_pre,
};

typedef void (*parser_char_handler)(void* parser,char c,long current_loc);

typedef struct{
//	_IN FILE *file;
	_IN char* file_path;
	//_IN char* pBuffer;
	_IN parse_area part;
}simple_parse_arg;

typedef struct{
	enum simple_state st;
	stackT *start_tags;
	Tag_Array *array_tags;
	Tag_Array *un_start_tags;
	parser_char_handler char_handler;

	unsigned long start_tag_num;
	unsigned long end_tag_num;
	unsigned long tasks;

	int unresolved_end_tag;
	int unresolved_start_tag;


	unsigned long id_base;
	tag_info* tag_tmp;
}simple_parser;



typedef void (*parser_end_tag_handler)(tag_info* end_tag, simple_parser* parser);


void* simple_parse(void* arg);
Tag_Array* parser_merge(simple_parser** parsers, int num);
#endif
