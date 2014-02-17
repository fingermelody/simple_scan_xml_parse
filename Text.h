#ifndef STRING_H
#define STRING_H
#define MAX_TEXT_LENGTH 1024*1024*80
typedef struct{
	char *chars;
	int index;
}Text;

void text_init(Text* text);
void text_add_char(Text* text, char c);
#endif
