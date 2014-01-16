#include "Text.h"
void text_init(Text* text){
	memset(text->chars,0);
	text->index = 0;
}

void text_add_char(Text* text, char c){
	text->chars[text->index++] = c;
}
