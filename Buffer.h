#pragma once
#include <stdio.h>

typedef struct Buffer {
	size_t size;
	size_t length;
	size_t filelength;
	char * data;
	FILE * file;
} Buffer;

Buffer * New();
Buffer * New(const char * filename);
void Destroy(Buffer * b);

//Add data to buffer
bool BufferAdd(Buffer * b, const void * data, size_t len);
//Add t into b
bool BufferAdd(Buffer * b, Buffer * t);
//Format into b
bool BufferFormat(Buffer * b, const char * format, ...);
//attempt pre-allocating memory by size
bool PreAlloc(Buffer * b, size_t size);
//return data as a const char *, if this is a file, the data will be loaded to memory
char * c_str(Buffer * b);