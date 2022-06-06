#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "common_includes.h"
#include "common_utils.h"

typedef struct DnsHeader {
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
} dns_header_t;

SOCKET sock;

char far* revert_question_name(unsigned char* reader, unsigned char* response, int* count);
size_t change_question_name(const unsigned char* hostname, unsigned char* qname);
int validateHost(const unsigned char* hostname);
struct hostent* dnsQuery(const char* hostname);
char* createDnsQueryBuf(const char* hostname, size_t* p_sizeof_query);
struct hostent* parseDnsResponseBuf(const unsigned char* response);
void printRemoteHost(struct hostent* remoteHost);
void assertDnsQueryResultIsValid(const struct hostent* remoteHost, const char* hostname);
char far* revert_question_name(unsigned char* reader, unsigned char* response, int* count);
size_t read_qname(const unsigned char* reader, char far** h_name);
void parseDnsHeaderFromResponse(dns_header_t* dns);


typedef struct Question {
    unsigned short qtype;
    unsigned short qclass;
} question_t;

typedef struct Response {
    uint16_t rtype;
    uint16_t rclass;
    uint32_t rttl;
    uint16_t rdlength;
    char char_1;
    char char_2;
    char char_3;
    char char_4;
    char char_5;
    char char_6;
    char char_7;
    char char_8;
    struct in_addr addr;
} response_t;


typedef struct Query {
    unsigned char* name;
    struct question* ques;
} query_t;

typedef struct Answer {
    unsigned char* rcode;
    unsigned char* address;

} answer_t;