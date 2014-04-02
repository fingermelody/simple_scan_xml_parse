#include "TagArray.h"
#include "stdafx.h"
void tag_array_add(Tag_Array* array, tag_info *info){

	array->tags[array->index ++] = *info;
}

void tag_array_init(Tag_Array* array){
	array->tags = (tag_info*)malloc(sizeof(tag_info)*1024*1024*8);
	array->index = 0;
}
