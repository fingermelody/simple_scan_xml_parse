#include "stdafx.h"
#include <mpi.h>
#include "slaves.h"
#include "array.h"
#include "MPI_MSG_TYPE.h"
void* query_tag_id(char* tag_name){
	MPI_Status status;

	if(tag_name == NULL){
		printf("error: tag name null");
		return NULL;
	}
	memcpy(q_tag_name,tag_name,sizeof(q_tag_name));
	int i;//temporary variable
	for(i=1;i<=slaves_num;i++){
		MPI_Send(q_tag_name,sizeof(q_tag_name),MPI_CHAR,i,MSG_QUERY_REQUEST,MPI_COMM_WORLD);
	}
	return NULL;
}

//void* query(void* argc){
//	char tag_name[20];
//	while(1)
//	{
//		fgets(tag_name,sizeof(tag_name),stdin);
//		array* array_ids = query_tag_id(tag_name);
//		int* ids = (int*)array_ids->data;
//		int i;
//		for(i=0;i<array_ids->size;i++)
//			printf("%d ",ids[i]);
//	}
//
//	return NULL;
//}
