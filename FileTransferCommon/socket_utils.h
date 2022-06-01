#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "common_includes.h"

boolean socket_initialize(WSADATA* wsaData);
int socket_connect(SOCKET* sock, const char* dest, const u_short port);
int socket_listen(SOCKET* sock, SOCKADDR_IN* sock_addr, const uint16_t port);
int safe_recv(SOCKET* sock, char* buf, int len);
void safe_send(SOCKET* sock, char* buf, int len);