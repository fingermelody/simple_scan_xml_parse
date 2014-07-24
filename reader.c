#include "stdafx.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "reader.h"
#include "task_queue.h"

//read_buf buffer;


void reader_init(reader* freader,_IN simple_parser* parser,_IN char* file_path, _IN long offset,_IN long length){
	freader->file_path = file_path;
	freader->length = length;
	freader->offset = offset;
	freader->parser = parser;
	freader->buffer = (read_buf*)malloc(sizeof(read_buf));
	memset(freader->buffer->buf,0,BUF_SIZE);
	freader->buffer->index = 0;
	freader->pf = open(freader->file_path,S_IRUSR);
}


int reader_read_and_handle(reader* freader){

	int chunks = freader->length/BUF_SIZE;
	int i,j;
	long current, offset;
	offset = freader->offset;
	for(i=0;i<chunks;i++){
		pread(freader->pf,freader->buffer->buf,BUF_SIZE,offset+i*BUF_SIZE);
		freader->buffer->size = BUF_SIZE;
		if(freader->parser != NULL){
			for(j=0;j<freader->buffer->size;j++){
				current = offset +i*BUF_SIZE + j;
				if(freader->buffer->buf[j]==EOF) break;
				freader->parser->char_handler(freader->parser,freader->buffer->buf[j],current);
			}
		}
	}
	if(freader->length%BUF_SIZE != 0){
		freader->buffer->size = freader->length%BUF_SIZE;
		pread(freader->pf,freader->buffer->buf,freader->buffer->size, offset + i*BUF_SIZE);
		if(freader->parser != NULL){
			for(j=0;j<freader->buffer->size;j++){
				current = offset + i*BUF_SIZE + j;
				if(freader->buffer->buf[j]==EOF) break;
				freader->parser->char_handler(freader->parser,freader->buffer->buf[j],current);
			}
		}
	}
	return 0;
}

int reader_seam(reader* freader){

	int seam_over=0, i=0, j;
	long current;
	long offset = freader->offset + freader->length;
	do{
		if(freader->parser->st == st_Content)
			break;
		freader->buffer->size = BUF_SIZE;
		pread(freader->pf,freader->buffer->buf,BUF_SIZE,offset+i*BUF_SIZE);
		if(freader->parser != NULL){
			for(j=0;j<freader->buffer->size;j++){
				current = offset + i*BUF_SIZE + j;
				if(freader->buffer->buf[j] == EOF){
					break;
					seam_over = 1;
				}
				freader->parser->char_handler(freader->parser,freader->buffer->buf[j],current);
				if(freader->parser->st == st_Content){
					seam_over = 1;
					break;
				}
			}
		}
		i++;
	}while(!seam_over);

	simple_parser* parser = freader->parser;
	while(!StackIsEmpty(parser->start_tags)){
		tag_info *s_tag = (tag_info*)StackPop(parser->start_tags);
		tag_info* p = (tag_info*)StackGetTop(parser->start_tags);
		if (p == NULL)
			s_tag->parent = 0- parser->unresolved_end_tag;
		else
			s_tag->parent = p->id;
		tag_array_add(parser->array_tags,s_tag);
		tag_array_add(parser->un_start_tags,s_tag);
		parser->end_tag_num++;
		if(parser->end_tag_num%TAGS_PER_TIME == 0){
			parse_task *task = (parse_task*)malloc(sizeof(parse_task));
			task->info = &(freader->parser->array_tags->tags[parser->tasks*TAGS_PER_TIME]);
			task->tag_num = TAGS_PER_TIME;
			add_task(task);
			parser->tasks++;
		}
	}

	if(parser->end_tag_num%TAGS_PER_TIME != 0){
		parse_task *task = (parse_task*)malloc(sizeof(parse_task));
		task->info = &(freader->parser->array_tags->tags[parser->tasks*TAGS_PER_TIME]);
		task->tag_num = freader->parser->end_tag_num%TAGS_PER_TIME;
		add_task(task);
		parser->tasks++;
	}

	return 0;
}

int reader_destroy(reader* freader){
	close(freader->pf);
	free(freader->buffer);
	return 0;
}

