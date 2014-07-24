#include "array.h"

int array_sum(array** arrays,int num){
	int i,sum=0;
	for(i=0;i<num;i++){
		sum = arrays[i]->size;
	}
	return sum;
}

void array_init(array* a){
  a->data = (void*)malloc(sizeof(void*));
  a->size = 0;
}
