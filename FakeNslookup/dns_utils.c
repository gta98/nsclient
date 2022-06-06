#include "dns_utils.h"


struct hostent* dnsQuery(const char* hostname) {
    /*
    * INPUT: "hostname": e.g. "google.com", "www.ynet.co.il"
    * RETURN: a hostent object == gethostbyname(hostname)
    */
    struct hostent* remoteHost;
    int i, status;
    unsigned char* query;
    unsigned char* response;
    size_t sizeof_query, sizeof_response;
    answer_t* ans;
    unsigned char* reader;

    remoteHost = NULL;
    query = NULL;
    response = NULL;
    sizeof_query = 0;

#if FLAG_REAL_NSLOOKUP == 1
    remoteHost = gethostbyname(hostname);
#else

    query = createDnsQueryBuf(hostname, &sizeof_query);
    if (!query) {
        printd("Could not create DNS query buffer!\n");
        goto dnsQueryFailure;
    }
    assertd(sizeof_query > 0);
    printd("Printing query:\n");
    printAsBytes(query, sizeof_query);

    status = sendto(sock, query, (int)sizeof_query, 0, NULL, 0);
    if (status == SOCKET_ERROR) {
        printd("Could not send query to DNS server due to socket error\n");
        goto dnsQueryFailure;
    }

    sizeof_response = SIZE_DNS_RESPONSE_BUF;
    response = malloc(sizeof(char) * ((size_t)sizeof_response + (size_t)1));
    if (!response) {
        printd("Could not allocate DNS response buffer!\n");
        goto dnsQueryFailure;
    }
    status = recvfrom(sock, response, (int)sizeof_response, 0, NULL, 0);
    if (status == SOCKET_ERROR) {
        printd("Failure in receiving response from DNS server - status %d\n last error %ld", status, WSAGetLastError());
        goto dnsQueryFailure;
    }
    sizeof_response = status; /* actual response size */

    remoteHost = parseDnsResponseBuf(response, sizeof_response);
    
    if (!remoteHost) {
        printd("Failure in parsing response from DNS server!\n");
        goto dnsQueryFailure;
    }
#endif

dnsQueryFinish:
    if (query) free(query);
    if (response) free(response);
    return remoteHost;

dnsQueryFailure:
    if (remoteHost) {
        free(remoteHost);
        remoteHost = NULL;
    }
    goto dnsQueryFinish;
}





char* createDnsQueryBuf(const char* hostname, size_t* sizeof_query) {
    /*
    * INPUT: "hostname": e.g. "google.com", "www.ynet.co.il"
    * RETURN: "query[]":
    *         This string is sent to the DNS server
    *         It contains the request "give me the IP address for <hostname>"
    */
    char* buf;
    dns_header_t *dns;
    size_t sizeof_qname;
    char* qname;
    question_t *qinf;

    *sizeof_query = sizeof(dns_header_t) + (sizeof(char) * 1024) + sizeof(question_t); // FIXME - set constant SIZE_DNS_QUERY_BUF;
    buf = NULL;
    buf = malloc(sizeof(char) * (*sizeof_query));
    if (!buf) {
        return NULL;
    }

    dns = (dns_header_t*)(buf + 0);
    dns->id = (unsigned short)htons(1);
    dns->qr = 0;
    dns->opcode = 0;
    dns->aa = 0;
    dns->tc = 0;
    dns->rd = 1;
    dns->ra = 0;
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1);
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    qname = (char*) (buf + sizeof(dns_header_t));
    sizeof_qname = (sizeof(char))*change_question_name(hostname, qname);
    *sizeof_query = (*sizeof_query) - (sizeof(char) * 1024) + sizeof_qname;

    qinf = (question_t*)(buf + sizeof(dns_header_t) + sizeof_qname);
    qinf->qtype = htons(1);
    qinf->qclass = htons(1);

    printd("Final size is %zu\n", *sizeof_query);

    return buf;

}


size_t change_question_name(const unsigned char* _hostname, unsigned char* dst) {
    /*
    * INPUT: "_hostname": e.g. "www.abcd.com"
    * OUTPUT: "dst": e.g. [0x3,"www",0x4,"abcd",0x3,"com",0x0]
    * RETURN: size of dst written
    */
    int i, j;
    size_t len_original, seg_len, dst_counter;
    char* token;
    char* hostname;

    len_original = strlen(_hostname);
    hostname = _strdup(_hostname);
    if (!hostname) return 0;
    dst_counter = 0;
    token = strtok(hostname, ".");
    while (token != NULL) {
        seg_len = strlen(token);
        assertd((1 <= seg_len) && (seg_len <= 63)); /* Already verified earlier */
        dst[dst_counter] = (unsigned char) seg_len;
        dst_counter += sizeof(char);
        for (j = 0; j < seg_len; j++)
            dst[dst_counter+j] = token[j];
        dst_counter += seg_len;
        token = strtok(NULL, ".");
    }
    dst[dst_counter] = 0;
    dst_counter += 1;

    printd("bytes([");
    for (i = 0; i < dst_counter-1; i++) {
        printd("0x%02x,", dst[i]);
    }
    printd("0x%02x])", dst[dst_counter - 1]);

    free(hostname);

    return dst_counter;
}


struct hostent* parseDnsResponseBuf(const unsigned char* response, size_t sizeof_response) {
    /*
    * INPUT: "response": fetched from DNS server through recvfrom()
    * RETURN: hostent object with returned IP
    */
    struct hostent* remoteHost;
    unsigned char* reader;
    dns_header_t* dns;
    response_t* response_section;
    int i, j;
    size_t sizeof_qname;
    char* data_bla;
    uint8_t address_octet;
    char* aliases;
    struct in_addr* yosi;

    printd("About to print response\n");
    printAsBytes(response, sizeof_response);
    printd("About to parse remoteHost from response\n");
    
    remoteHost = malloc(sizeof(struct hostent));
    if (!remoteHost) return NULL;

    reader = response + 0;

    dns = (dns_header_t*)reader;
    parseDnsHeaderFromResponse(dns);
    reader += sizeof(dns_header_t);
    
    aliases = NULL;
    remoteHost->h_aliases = &aliases;
    remoteHost->h_length = 4;// dns->ans_count;
    remoteHost->h_addrtype = 1; /* IP type */
    remoteHost->h_addr_list = malloc(sizeof(char*) * 1);
    if (!remoteHost->h_addr_list) {
        free(remoteHost);
        return NULL;
    }
    
    sizeof_qname = read_qname(reader, &remoteHost->h_name);
    reader += sizeof_qname;
    printAsBytes(reader, sizeof(response_t));

    uint16_t rtype = ntohs((reader[0] << 4) | (reader[1]));
    reader += 2 * sizeof(char);
    uint16_t rclass = ntohs((reader[0] << 4) | (reader[1]));
    reader += 2 * sizeof(char);
    uint32_t rttl = (reader[0] << 12) | (reader[1] << 8) | (reader[2] << 4) | (reader[3] << 0);
    reader += 4 * sizeof(char);
    uint16_t rdlength = (reader[0] << 4) | (reader[1]);
    reader += 2 * sizeof(char);

    reader += 6 * sizeof(char);
    yosi = (struct in_addr*)reader;

    printf("sizeof char: %d\n", sizeof(char));
    printf("specifically size of response_t is:::::: %d\n", sizeof(response_t));
    printf("total size of structs: %d\n", sizeof(dns_header_t) + sizeof_qname + sizeof(response_t));
    remoteHost->h_addr_list[0] = inet_ntoa(*yosi);
    printf("BIBI: %s\n", remoteHost->h_addr_list[0]);

    return remoteHost;
}


size_t read_qname(const unsigned char* reader, char far** h_name) {
    // temporary solution
    int i, j;
    unsigned char next_up;
    size_t sizeof_qname;
    size_t sizeof_h_name;
    char* ptr_to_h_name;

    sizeof_qname = strlen(reader) + sizeof(char); // to account for null terminator
    sizeof_h_name = sizeof_qname - sizeof(char); // to remove leading counter

    *h_name = malloc(sizeof_h_name);
    if (!(*h_name)) return 0;

    ptr_to_h_name = *h_name;
    next_up = *reader;
    reader++;
    while (next_up) {
        for (i = 0; i < next_up; i++) {
            ptr_to_h_name[0] = *reader;
            ptr_to_h_name++;
            reader++;
        }
        next_up = *reader;
        if (next_up) {
            ptr_to_h_name[0] = '.';
            ptr_to_h_name++;
        }
        else {
            ptr_to_h_name[0] = '\0';
            ptr_to_h_name++;
        }
        reader++;
    }

    return sizeof_qname;
}


void parseDnsHeaderFromResponse(dns_header_t* dns) {
    printf("\nThe response contains : ");
    printf("\n %d Questions.", ntohs(dns->q_count));
    printf("\n %d Answers.", ntohs(dns->ans_count));
    printf("\n %d Authoritative Servers.", ntohs(dns->auth_count));
    printf("\n %d Additional records.\n\n", ntohs(dns->add_count));

    // check R-code // TODO - maybe put in a function

    switch (dns->rcode) {
    case 0:
        break;
    case 1: perror("Format error - The name server was unable to interpret the query\n");
        break;
    case 2: perror("Server failure - The name server was unable to process this query due to a problem with the name server.\n");
        break;
    case 3: perror("Name Error - Meaningful only for responses from an authoritative name server, this code signifies that the domain name referenced in the query does not exist.\n");
        break;
    case 4: perror("Not Implemented - The name server does not support the requested kind of query\n");
        break;
    case 5: perror("Refused - The name server refuses to perform the specified operation for policy reasons.\n");
        break;
    default: perror("Reserved for future use.\n");
        break;
    }
}


int validateHost(const unsigned char* _hostname) {
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
    int i;
    char* label;
    char* hostname;
    size_t label_length, length;

    length = strlen(_hostname);

    if ((length == 0) || (length > 255)) {
        /* RFC 1035 states:
            the total length of a domain name (i.e., label octets and label length octets)
            is restricted to 255 octets or less.
        */
        return STATUS_ERR_BAD_NAME;
    }

    hostname = malloc(length+sizeof(char));
    if (!hostname) return STATUS_ERR_MALLOC_BUF;

    for (i = 0; _hostname[i]; i++) hostname[i] = _hostname[i];
    hostname[length] = 0;

    label = strtok(hostname, ".");
    while (label) {
        label_length = strlen(label);
        if ((label_length < 1) || (label_length > 63)) {
            /* RFC 1035 states:
                2.3.4. Size limits
                Various objects and parameters in the DNS have size limits.  They are
                listed below.  Some could be easily changed, others are more
                fundamental.
                labels          63 octets or less
               And, from the definition <label>, it must have at least 1 letter (@ idx 0)
            */
            return STATUS_ERR_BAD_NAME;
        }
        /* RFC 1035 states:
            <label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]
        */
        if (!is_letter(label[0])) {
            return STATUS_ERR_BAD_NAME;
        }
        if (!str_check_all(label, is_let_dig_hyp)) {
            return STATUS_ERR_BAD_NAME;
        }
        if (!is_let_dig(label[label_length - 1])) {
            return STATUS_ERR_BAD_NAME;
        }
        label = strtok(NULL, ".");
    }

    free(hostname);

    return STATUS_SUCCESS;
}

void printRemoteHost(struct hostent* remoteHost) {
    struct in_addr addr;
    if (remoteHost
        && (remoteHost->h_addrtype == AF_INET)
        && (remoteHost->h_addr_list[0] != 0)) {
        addr.s_addr = *(u_long*)remoteHost->h_addr_list[0];
        printf("%s\n", inet_ntoa(addr));
    }
    else {
        printf(MSG_ERR_NONEXISTENT);
    }
}

void assertDnsQueryResultIsValid(const struct hostent* remoteHost, const char* hostname) {
#if FLAG_DEBUG == 1
    int i;
    struct hostent* remoteHostDebug;

    remoteHostDebug = gethostbyname(hostname);
    if (remoteHostDebug != NULL) assertd(remoteHost != NULL);
    if (remoteHost != NULL) assertd(remoteHostDebug != NULL);
    if (remoteHost && remoteHostDebug) {
        //printf("remoteHost->h_name=%s\nremoteHostDebug->h_name")
        assertd(strcmp(remoteHost->h_name, remoteHostDebug->h_name) == 0);
        assertd(remoteHost->h_length == remoteHostDebug->h_length);
        assertd(remoteHost->h_addrtype == remoteHostDebug->h_addrtype);
        for (i = 0; (remoteHost->h_addr_list[i] || remoteHostDebug->h_addr_list[i]); i++) {
            assertd(remoteHost->h_addr_list[i] && remoteHostDebug->h_addr_list[i]);
            assertd(strcmp(remoteHost->h_addr_list[i], remoteHostDebug->h_addr_list[i]) == 0);
        }
        for (i = 0; (remoteHost->h_aliases[i] || remoteHostDebug->h_aliases[i]); i++) {
            assertd(remoteHost->h_aliases[i] && remoteHostDebug->h_aliases[i]);
            assertd(strcmp(remoteHost->h_aliases[i], remoteHostDebug->h_aliases[i]) == 0);
        }
    }
#else
    return;
#endif
}