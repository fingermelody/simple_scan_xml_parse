#ifndef MPQ_HASH_H
#define MPQ_HASH_H
#include <stddef.h>

typedef struct {
	unsigned long dwHash1;
	unsigned long dwHash2;
}MPQ_KEY;

//void prepareCryptTable();
//size_t hash_map_MPQ_hash_func(const void *key, size_t capacity);
//int MPQ_comparator(const void* l, const void * r);
void* MPQ_Generate_Key(char* string);
#endif
