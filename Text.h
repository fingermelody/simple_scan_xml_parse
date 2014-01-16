#ifndef STRING_H
#define STRING_H

typedef struct{
	char chars[1024*1024*8];
	int index;
}Text;

void text_init(Text* text);
void text_add_char(Text* text, char c);
#endif
