#include "Text.h"
void text_init(Text* text){
	text->offset = 0;
	text->length = 0;
}
void text_set(Text* text,int pre_pos,int cur_pos){
	text->length = cur_pos - pre_pos;
	text->offset = pre_pos;
}
int text_size(Text* text){
	return text->length;
}
