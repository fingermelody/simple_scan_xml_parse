#ifndef NODE_H
#define NODE_H
#include "tag.h"
typedef struct _node
{
	Tag *start_tag;
	Tag *end_tag;
	Tag *parent;
}Node;

void node_init(Node *node);
#endif