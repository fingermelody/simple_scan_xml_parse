/*
#ifndef SIMPLE_STATE_MACHINE_H
#define SIMPLE_STATE_MACHINE_H

#include "stdafx.h"

#define TAGS_PER_TIME 1024
#define NODES_PER_TIME 1024
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

typedef struct{
//	_IN FILE *file;
	_IN char* file_path;
	//_IN char* pBuffer;
	_OUT tag_info **s_tags_ready;
	_OUT Text **text_read;
}simple_parse_arg;

int file_read_over;


#endif
*/
