#include "TagArray.h"
#include "stdafx.h"
void tag_array_add(Tag_Array array, Tag *tag){
	array.tags[++array.index] = *tag;
}

void tag_array_init(Tag_Array array){
	array.index = 0;
}