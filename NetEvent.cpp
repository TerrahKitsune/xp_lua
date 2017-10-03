#include "NetEvent.h"
#include <stdlib.h>
#include <string.h>

NetEvent * NetEvent_Create(SOCKET s, int type, const char * data, int len) {

	NetEvent * ndata = (NetEvent*)malloc(sizeof(NetEvent) + len);
	if (!ndata)
		return NULL;

	ndata->s = s;
	ndata->type = type;
	if(len > 0)
		memcpy(ndata->data, data, len);
	ndata->len = len;

	return ndata;
}