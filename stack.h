#ifndef STACK_H
#define STACK_H

typedef char stackElementT;

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

#endif
