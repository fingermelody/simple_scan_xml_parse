#ifndef TAG_H
#define TAG_H
#define MAX_STRING 30
#define MAX_ATTRIBUTE_NUM 5

struct _attribute{
	char name[MAX_STRING];
	char value[MAX_STRING];
	int nameCharIndex;
	int valueCharIndex;
};
typedef struct _attribute attribute;

typedef struct _tag{
	int id;
	int location;
	int lengh;
	int parent;

	char name[MAX_STRING];
	int numOfAttribute;
	attribute attributes[MAX_ATTRIBUTE_NUM];
	int nameCharIndex;
	int attrIndex;
}Tag;

void tag_init(Tag *tag);
void tag_destroy(Tag *tag);
#endif