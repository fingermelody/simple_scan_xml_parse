#ifndef SLAVES_H
#define SLAVES_H

#define MAX_SLAVES_NUM 4

int slaves_num;
#include "tools/hash_map.h"
/**************************************************/
/*values for tag's inverted index.*/

hash_map *tags_inverted_index;
/**************************************************/


/**************************************************/
/*values for master to control the slaves*/
int idle;// 1 means idle, 0 means busy
int idle_node[5];// store the idle state of slaves
int slave_parse_stop;// 0 means slaves are running parse work, 1 means stop.
int slave_query_work;// 0 means slaves are running query work, 1 means stop.
int response_count[5];// store the count of data need to receive
/**************************************************/

/**************************************************/
/*values for query*/
char q_tag_name[20];

/***************************************************/

#endif
