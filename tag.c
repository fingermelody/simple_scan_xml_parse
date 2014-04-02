#include "tag.h"
#include "stdlib.h"

void tag_info_init(tag_info* info){
	if(info == NULL)
		info = (tag_info*)malloc(sizeof(tag_info));
	info->id = 0;
	info->lengh = 0;
	info->location = 0;
	info->parent = 0;
}

void free_tag_info(tag_info* info){
	if(info == NULL) return;
	free(info);
}

void tag_init(Tag *tag){
	if(tag == NULL)
		tag = (Tag*)malloc(sizeof(Tag));
	tag_info_init(&(tag->info));
	tag->nameCharIndex = 0;
	tag->attrIndex = 0;
	tag->numOfAttribute = 0;
}

void tag_destroy(Tag *tag){
	free(tag);
}
