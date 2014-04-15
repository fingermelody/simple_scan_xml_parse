#include <mpi.h>
#include "tools/hash_map.h"
#include "tools/linked_list.h"
#include "slaves.h"
#include "MPI_MSG_TYPE.h"
#include "tag.h"
linked_list* slave_query_ids(char* tag_name){
	linked_list* id_list = (linked_list*)hash_map_get(tags_inverted_index, tag_name);
	return id_list;
}

void slave_query(int count){
	char tag_name[MAX_STRING];
	MPI_Recv(tag_name, count, MPI_CHAR,0,MSG_QUERY_REQUEST,MPI_COMM_WORLD,MPI_STATUSES_IGNORE);
	linked_list* tags = (linked_list*)hash_map_get(tags_inverted_index,tag_name);
	linked_list_node* head = linked_list_head(tags);
	size_t size = linked_list_size(tags);
	int *result = (int*)malloc(sizeof(int)*size);
	int i = 0;
	while(head){
		result[i] = ((Tag*)head->data)->info.id;
		i++;
		head = head->next;
	}
	MPI_Send(result,size,MPI_INT,0,MSG_QUERY_RESPONSE,MPI_COMM_WORLD);
}

