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

int removeSignificantBit(int num);
size_t change_question_name(const unsigned char* hostname, unsigned char* qname);
int validateHost(const unsigned char* hostname);
struct hostent* dnsQuery(const char* hostname);
char* createDnsQueryBuf(const char* hostname, size_t* p_sizeof_query, size_t* sizeof_qname);
struct hostent* parseDnsResponseBuf(const unsigned char* response, size_t sizeof_reponse, size_t sizeof_qname);
void printRemoteHost(struct hostent* remoteHost);
void assertDnsQueryResultIsValid(const struct hostent* remoteHost, const char* hostname);
size_t read_qname(const unsigned char* reader, char far** h_name);
int read_qname_wrapper(const unsigned char* reader, size_t sizeof_qname, size_t sizeof_response,
const unsigned char* response, struct hostent* remoteHost);
void parseDnsHeaderFromResponse(dns_header_t* dns);

typedef struct Name {
    unsigned char label_pointer: 2;
    unsigned char* offset;

} name_t;

typedef struct Question {
    unsigned short qtype;
    unsigned short qclass;
} question_t;

typedef struct Response {
   // struct Name rname; // compressed name
    unsigned short rtype;
    unsigned short rclass;
    unsigned int rttl;
    unsigned short rdlength;
    struct in_addr addr;
} response_t;


typedef struct Query {
    unsigned char* name;
    struct question_t* ques;
} query_t;

