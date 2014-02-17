#include "tag.h"
#include "stdlib.h"
void tag_init(Tag **tag){
	*tag = (Tag*)malloc(sizeof(Tag));
	(*tag)->lengh = 0;
	(*tag)->location = 0;
	(*tag)->nameCharIndex = 0;
	(*tag)->attrIndex = 0;
}

void tag_destroy(Tag *tag){
	free(tag);
}
