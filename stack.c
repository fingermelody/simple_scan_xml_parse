#include "stack.h"
#include <stdio.h>
void StackInit(stackT *stackP, int maxSize){

	stackElementT *newContents;

  /* Allocate a new array to hold the contents. */
  newContents = (stackElementT *)malloc(sizeof(stackElementT)
										* maxSize);

  if (newContents == NULL) {
	fprintf(stderr, "Insufficient memory to initialize stack.\n");
	exit(1);  /* Exit, returning error code. */
  }

  stackP->contents = newContents;
  stackP->maxSize = maxSize;
  stackP->top = -1;  /* I.e., empty */
}

void StackDestroy(stackT *stackP){

	free(stackP -> contents);
	stackP -> contents = NULL;
	stackP -> maxSize = 0;
	stackP -> top = -1;
}

int StackIsEmpty(stackT *stackP){
	return stackP->top < 0;
}

int StackIsFull(stackT *stackP){
	return stackP->top >= stackP->maxSize - 1;
}

void StackPush(stackT *stackP, stackElemnetT element){

	if (StackIsFull(stackP)) {
		fprintf(stderr, "Can't push element on stack: stack is full.\n");
		exit(1);  /* Exit, returning error code. */
	  }

	stackP->contents[++stackP->top] = element;
}

stackElementT StackPop(stackT *stackP){
	if (StackIsFull(stackP)) {
	    fprintf(stderr, "Can't push element on stack: stack is full.\n");
	    exit(1);  /* Exit, returning error code. */
	  }
	return stackP->contents[stackP->top --];
}
