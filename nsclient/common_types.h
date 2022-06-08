#pragma once
#include <stdint.h>

#define bit unsigned char
#define byte unsigned char
//#define ull unsigned long long

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
// Contains DNS header struct, used for writing query buffer, and reading from response

typedef struct Question {
    unsigned short qtype;
    unsigned short qclass;
} question_t;
// Contains question, used for reading from response buffer after header 

typedef struct Response {
    unsigned short rtype;
    unsigned short rclass;
    unsigned int rttl;
    unsigned short rdlength;
    struct in_addr addr;
} response_t;
// Contains response