#ifndef TAG_ARRAY_H
#define TAG_ARRAY_H
#include "tag.h"

typedef struct _tag_array
{
	tag_info *tags;
	int* map;
	int index;
}Tag_Array;

void tag_array_init(Tag_Array *array);
void tag_array_add(Tag_Array *array,tag_info* tag);
#endif
