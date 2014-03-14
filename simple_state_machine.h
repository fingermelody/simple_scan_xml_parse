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
};

typedef struct{
//	_IN FILE *file;
	_IN char* file_path;
	//_IN char* pBuffer;
	_OUT Tag **s_tags_ready;
	_OUT char **string_read;
}simple_parse_arg;

int file_read_over;


#endif
