#include <string.h>
#include "array.h"

parse_query(char* str, array* condition){
	char* p = str;
	char** r = (char**)malloc(sizeof(char*)*5);
	int i = 0;
	r[i++] = p;
	while(p=strchr(p,'/')){
		*p = '\0';
		p++;
		r[i++] = p;
	}
	condition->data = r;
	condition->size = i;
}


