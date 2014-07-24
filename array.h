#ifndef ARRAY_H
#define ARRAY_H

typedef struct{
	void* data;
	int size;
}array;

int array_sum(array** arrays,int num);

#endif
