#include "networking.h"
#include <stdio.h>

int GetIP(SOCKET s, char * buffer, size_t max) {

	sockaddr res = { 0 };
	int size = sizeof(sockaddr);
	if (getpeername(s, (sockaddr*)&res, &size) != 0) {
		return 0;
	}
	else {
		int port;
		switch (res.sa_family)
		{
		case AF_INET: {
			struct sockaddr_in *addr_in = (struct sockaddr_in *)&res;
			port = addr_in->sin_port;
			inet_ntop(AF_INET, &(addr_in->sin_addr), buffer, max);
			sprintf(buffer, "%s:%d", buffer, port);
			return 1;
		}
		case AF_INET6: {
			struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&res;
			port = addr_in6->sin6_port;
			inet_ntop(AF_INET6, &(addr_in6->sin6_addr), buffer, max);
			sprintf(buffer, "%s:%d", buffer, port);
			return 1;
		}
		default:
			break;
		}
	}

	return 0;
}