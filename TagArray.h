#ifndef TAG_ARRAY_H
#define TAG_ARRAY_H
#include "tag.h"

typedef struct _tag_array
{
	Tag tags[1024*1024];
	int index;
}Tag_Array;

void tag_array_init(Tag_Array array);
void tag_array_add(Tag_Array array,Tag* tag);
#endif