#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "common_includes.h"

SOCKET sock;

int change_question_name(const char* hostname, unsigned char* qname);
int validateHost(const char* hostname);
struct hostent* dnsQuery(const char* hostname);
char* createDnsQueryBuf(const char* hostname, size_t* p_sizeof_query);
struct hostent* parseDnsResponseBuf(const char* response);
void printRemoteHost(struct hostent* remoteHost);
void assertDnsQueryResultIsValid(const struct hostent* remoteHost, const char* hostname);