#include "TagArray.h"
#include "stdafx.h"
void tag_array_add(Tag_Array* array, tag_info *info){
	array->map[info->id] = array->index;
	array->tags[array->index ++] = *info;
	free(info);
}

void tag_array_init(Tag_Array* array){
	array->tags = (tag_info*)malloc(sizeof(tag_info)*1024*1024*8);
	memset(array->tags,0,sizeof(tag_info)*1024*1024*8);
	array->map = (int*)malloc(sizeof(int)*1024*1024*8);
	memset(array->map,0,sizeof(int)*1024*1024*8);
	array->index = 0;
}
