#include "node.h"
#include <stdlib.h>
void node_init(Node *node)
{
	node = (Node*)malloc(sizeof(Node));
}