#include <mpi.h>
#include "tools/hash_map.h"
#include "tools/linked_list.h"
#include "slaves.h"
#include "MPI_MSG_TYPE.h"
linked_list* slave_query_ids(char* tag_name){
	linked_list* id_list = (linked_list*)hash_map_get(tags_inverted_index, tag_name);
	return id_list;
}

void* slave_query_worker(void* argc){
	MPI_Status status;
	while(1){
		MPI_Probe(0,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
		switch(status.MPI_TAG){
		case MSG_QUERY_REQUEST:
		{
			MPI_Recv(q_tag_name,sizeof(q_tag_name),MPI_CHAR,0,MSG_QUERY_REQUEST,MPI_COMM_WORLD,&status);
			linked_list *result = slave_query_ids(q_tag_name);
			int *ids = (int*)malloc(result->size * sizeof(int));
			linked_list_node* head = linked_list_head(result);
			int i=0;
			while(head){
				memcpy(&(ids[i]),head->data,sizeof(int));
				head = head->next;
				i++;
			}
			MPI_Send(ids,result->size,MPI_INT,0,MSG_QUERY_RESPONSE,MPI_COMM_WORLD);
			break;
		}
		}
	}

	return NULL;
}
