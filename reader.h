#ifndef READER_H
#define READER_H
#define BUF_SIZE 1024

#include "simple_parser.h"
typedef void (*handle_char)(char* c);

typedef struct _read_buf{
	char buf[BUF_SIZE];
	int index;
	int size;
}read_buf;

typedef struct _reader{
	int pf;
	long offset;
	long length;
	char* file_path;
	read_buf* buffer;
	simple_parser* parser;
}reader;

#endif
