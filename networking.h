#pragma once
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include "Buffer.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <openssl/err.h>
#include <openssl/ssl.h>

#pragma comment(lib, "OpenSSL/libssl.lib")
#pragma comment(lib, "OpenSSL/libcrypto.lib")
#pragma comment(lib, "OpenSSL/openssl.lib")
