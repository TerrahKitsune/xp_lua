#define _CRT_SECURE_NO_WARNINGS
#include "Buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define FORMATBUFFERSIZE 1048576
#define INITIAL 1024


static char fbuffer[FORMATBUFFERSIZE];

static bool Resize(Buffer * b, size_t more){

	if (b->length + more < b->size)
		return true;

	void * temp = realloc(b->data, b->size + more + INITIAL);
	if (!temp)
		return false;

	b->data = (char*)temp;
	b->size = b->size + more + INITIAL;
	return true;
}

Buffer * New(){

	Buffer * temp = (Buffer *)calloc(1, sizeof(Buffer));
	if (temp){
		temp->data = (char*)calloc(INITIAL, sizeof(char));
		if (!temp->data){
			free(temp);
			return NULL;
		}
		else
			temp->size = INITIAL;
	}

	return temp;
}

Buffer * New(const char * filename){

	if (filename == NULL || filename[0] == '\0')
		return New();

	Buffer * temp = (Buffer *)calloc(1, sizeof(Buffer));
	if (!temp)
		return NULL;

	temp->file = fopen(filename, "wb+");
	if (!temp->file){
		free(temp);
		return NULL;
	}

	return temp;
}

void Destroy(Buffer * b){
	if (b){
		if (b->file)
			fclose(b->file);
		if (b->data)
			free(b->data);
		free(b);
	}
}

bool PreAlloc(Buffer * b, size_t size){
	
	long newsize = size - b->size;
	
	if (b->file || newsize <= 0)
		return true;
	
	return Resize(b, newsize);
}

bool BufferAdd(Buffer * b, Buffer * t){
	return BufferAdd(b, t->data, t->length);
}

bool BufferAdd(Buffer * b, const void * data, size_t len){

	if (b->file){
		size_t written = fwrite(data, 1, len, b->file);
		b->filelength += written;
		return written == len;
	}

	if (b == NULL || b->data == NULL)
		return false;
	else if (data == NULL || len <= 0)
		return true;
	else if (!Resize(b, len))
		return false;

	memcpy(&b->data[b->length], data, len);
	b->length = b->length + len;
	return true;
}

bool BufferFormat(Buffer * b, const char * format, ...){

	va_list args;
	va_start(args, format);
	int bytes = vsprintf_s(fbuffer, format, args);
	if (bytes < 0)
		return false;
	return BufferAdd(b, fbuffer, bytes);
}

char * c_str(Buffer * b){
	if (b->file){
		if (b->data){
			free(b->data);
			b->data = NULL;
			b->length = 0;
			b->size = 0;
		}

		long len = ftell(b->file);
		if (len < 0){
			return NULL;
		}

		void * temp = malloc(len + 1);
		if (!temp)
			return NULL;
		b->data = (char*)temp;
		b->size = len + 1;
		b->length = len + 1;
		b->data[len] = '\0';

		rewind(b->file);
		fread(b->data, 1, len, b->file);
		fseek(b->file, len, SEEK_SET);
	}

	return b->data;
}