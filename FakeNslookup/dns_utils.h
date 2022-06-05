#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "common_includes.h"

struct dns_header {
    unsigned short id;
    unsigned char rd : 1;
    unsigned char tc : 1;
    unsigned char aa : 1;
    unsigned char opcode : 4;
    unsigned char qr : 1;
    unsigned char rcode : 4;
    unsigned char cd : 1;
    unsigned char ad : 1;
    unsigned char z : 1;
    unsigned char ra : 1;

    unsigned short q_count;
    unsigned short ans_count;
    unsigned short auth_count;
    unsigned short add_count;
};


struct question {
    unsigned short qtype;
    unsigned short qclass;
};


typedef struct {
    unsigned char* name;
    struct question* ques;
}; query;


SOCKET sock;

int change_question_name(const char* hostname, unsigned char* qname);
int validateHost(const char* hostname);
struct hostent* dnsQuery(const char* hostname);
char* createDnsQueryBuf(const char* hostname, size_t* p_sizeof_query);
struct hostent* parseDnsResponseBuf(const char* response);
void printRemoteHost(struct hostent* remoteHost);
void assertDnsQueryResultIsValid(const struct hostent* remoteHost, const char* hostname);