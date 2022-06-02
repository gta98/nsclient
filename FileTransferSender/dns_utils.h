#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "common_includes.h"

SOCKET sock;

int validateHost(const char* hostname);
struct hostent* dnsQuery(const char* hostname);
int createDnsQueryBuf(const char* hostname, const char* query);
int parseDnsResponseBuf(const char* response, struct hostent* remoteHost);
void printRemoteHost(struct hostent* remoteHost);