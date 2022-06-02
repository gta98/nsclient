#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "common_includes.h"

boolean socket_initialize(WSADATA* wsaData);
int socket_connect(SOCKET* sock, const char* dest, const u_short port);