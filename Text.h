#ifndef STRING_H
#define STRING_H
#define MAX_TEXT_LENGTH 83886080//1024*1024*80
typedef struct{
	long offset;
	long length;
}Text;

//void text_init(Text* text);
//void text_add_char(Text* text, char c);
//int text_size(Text* text);
#endif
