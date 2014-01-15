#ifndef STACK_H
#define STACK_H
#include "tag.h"
typedef void* stackElementT;

typedef struct{
	stackElementT *contents;
	int top;
	int maxSize;
}stackT;

void StackInit(stackT *stackP, int maxSize);
void StackDestroy(stackT *stackP);
int StackIsEmpty(stackT *stackP);
int StackIsFull(stackT *stackP);
void StackPush(stackT *stackP, stackElementT element);
stackElementT StackPop(stackT *stackP);
stackElementT StackGetTop(stackT *stackP);
stackElementT stackGep(int index);
#endif
