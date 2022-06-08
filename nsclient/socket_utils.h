#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "common_includes.h"

int socket_initialize(WSADATA* wsaData);
/*
* INPUT: "wsaData": WSADATA pointer
* RETURN: int: same as WSAStartup documented in winsock2.h
* Wrapper for winsock2.h:WSAStartup with predefined parameters
*/

int socket_connect(SOCKET* sock, const char* dest, const u_short port);
/*
* INPUT: "sock": socket pointer to initialize and connect through
* INPUT: "dest": destination IP address
* INPUT: "port": destination port
* RETURN: connect() status code as defined by winsock2.h
* Attempts to create UDP connection to dest:port, sets timeout of 2sec for recvfrom
*/