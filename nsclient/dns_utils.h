#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "common_includes.h"
#include "common_utils.h"

// Contains global socket instance - our UDP socket is open all the time
SOCKET sock;

int validateHost(const unsigned char* hostname);
/*
* INPUT: hostname
* RETURN: STATUS_SUCCESS if hostname is valid, STATUS_ERR_BAD_NAME otherwise
* DESCRIPTION:
* Follows RFC 1035 specification.
* we consider hostname to be valid <=> hostname matches definition of <subdomain>
* We are intentionally ignoring the " " case
* Definitions, from the specification:
* Octet ::= 8 bits (1 char)
* <domain> ::= <subdomain> | " "
* <subdomain> ::= <label> | <subdomain> "." <label>
* <label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]
* <ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>
* <let-dig-hyp> ::= <let-dig> | "-"
* <let-dig> ::= <letter> | <digit>
* <letter> ::= any one of the 52 alphabetic characters A through Z in
*              upper case and a through z in lower case
* <digit> ::= any one of the ten digits 0 through 9
*/

struct hostent* dnsQuery(const char* hostname);
/*
* INPUT: "hostname": e.g. "google.com", "www.ynet.co.il"
* RETURN: a hostent object == gethostbyname(hostname)
*/

size_t change_question_name(const unsigned char* hostname, unsigned char* qname);
/*
* INPUT: "_hostname": e.g. "www.abcd.com"
* OUTPUT: "dst": e.g. [0x3,"www",0x4,"abcd",0x3,"com",0x0]
* RETURN: size of dst written
*/

char* createDnsQueryBuf(const char* hostname, size_t* p_sizeof_query, size_t* sizeof_qname);
/*
* INPUT: "hostname": e.g. "google.com", "www.ynet.co.il"
* RETURN: "query[]":
*         This string is sent to the DNS server
*         It contains the request "give me the IP address for <hostname>"
*/

struct hostent* parseDnsResponseBuf(const unsigned char* response, size_t sizeof_reponse, size_t sizeof_qname);
/*
* INPUT: "response": fetched from DNS server through recvfrom()
*        sizeof_response
*        sizeof_qname : size of the name **sent** to DNS
* RETURN: hostent object with returned IP
*/

size_t read_qname(const unsigned char* reader, char far** h_name);
/*
* INPUT: "reader": pointer to location in which we can expect the hostname in the response
*                  we expect 3www6google3com0 style formatting
* INPUT: "h_name": pointer to pointer in which we want to store the extracted hostname
* OUTPUT: "h_name": will be in www.google.com format
* RETURN: size of name as stored within reader (3www6google3com0 -> 16)
*/

int read_qname_wrapper(const unsigned char* reader, size_t sizeof_qname, size_t sizeof_response,
						const unsigned char* response, struct hostent* remoteHost);
/*
 * Wrapper for read_qname, to account for different location possibilities for hostname
*/

void printRemoteHost(struct hostent* remoteHost);
/*
* INPUT: "remoteHost": a hostent struct
* OUTPUT: prints first IP address listed in "remoteHost->h_addr_list" if nonnull
*/

void parseDnsHeaderFromResponse(dns_header_t* dns);
/*
 * (enabled iff FLAG_DEBUG == 1)
 * If DNS server returned status code >= 1, displays error
 */

void assertDnsQueryResultIsValid(const struct hostent* remoteHost, const char* hostname);
/*
* (enabled iff FLAG_DEBUG == 1)
* Verifies correctness of remoteHost through comparison against gethostnameaddr() result
*/