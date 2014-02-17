#include "Text.h"
void text_init(Text* text){
	text->chars = (char*)malloc(sizeof(char)*MAX_TEXT_LENGTH);
	memset(text->chars,0,MAX_TEXT_LENGTH);
	text->index = 0;
}

void text_add_char(Text* text, char c){
	text->chars[text->index++] = c;
}
