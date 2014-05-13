#include "TagArray.h"
#include "stdafx.h"


void tag_array_add(Tag_Array* array, tag_info *info){
	array->map[info->id] = array->index;
	array->tags[array->index ++] = *info;
}

void tag_array_add_array(Tag_Array* array, Tag_Array* array_to_add){

	int i;
	for(i=0;i<array_to_add->index;i++){
		array->map[array_to_add->tags[i].id] = array->index;
		array->tags[array->index ++] = array_to_add->tags[i];
	}
}

void tag_array_init(Tag_Array* array, int max_size){

	array->tags = (tag_info*)malloc(sizeof(tag_info)*max_size);
	memset(array->tags,0,sizeof(tag_info)*max_size);
	array->map = (int*)malloc(sizeof(int)*max_size);
	memset(array->map,0,sizeof(int)*max_size);
	array->index = 0;
}

Tag_Array* tag_array_merge_arrays(Tag_Array** arrays, int num){
	int i,count;
	for(i=0;i<num;i++)
		count += arrays[i]->index;
	Tag_Array* result = (Tag_Array*)malloc(sizeof(Tag_Array));
	tag_array_init(result, count);
	for(i=0;i<num;i++)
		tag_array_add_array(result,arrays[i]);
	return result;
}


