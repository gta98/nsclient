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
    dns_header_t* dns;
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
    printd("bytes([");
    for (i = 0; i < sizeof_query-1; i++)
        printd("0x%02x,", query[i]);
    printd("0x%02x])", query[sizeof_query-1]);
    printd("\n");

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
    response[sizeof_response] = 0;

    printd("About to print response\n");
    printd("bytes([");
    for (i = 0; i < sizeof_response-1; i++)
        printd("0x%02x,", response[i]);
    printd("0x%02x", response[sizeof_response - 1]);
    printd("])\n");
    printd("About to parse remoteHost from response\n");
    remoteHost = parseDnsResponseBuf(response, sizeof_response, sizeof_query);
    
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


struct hostent* parseDnsResponseBuf(const unsigned char* response, size_t sizeof_response, size_t sizeof_query) {
    /*
    * INPUT: "response": fetched from DNS server through recvfrom()
    * RETURN: hostent object with returned IP
    */
    struct hostent* remoteHost;
    unsigned char* reader;
    dns_header_t* dns;
    int i, j;
    
    remoteHost = malloc(sizeof(struct hostent));
    if (!remoteHost) return NULL;

    /* FIXME: Alon - Implement here - fill remoteHost based on DNS response */

    reader = response+sizeof_query;
    //response += sizeof_query;
    //sizeof_response -= sizeof_query;
    dns = (dns_header_t*)response;

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

    int stop = 0;
    remoteHost->h_name = revert_question_name(reader, response, &stop);
    printd("h_name == %s\n", remoteHost->h_name);

    return remoteHost;
}


char far* revert_question_name(unsigned char* reader, unsigned char* response, int* count) {
    unsigned char* name;
    unsigned int p = 0, jumped = 0, offset;
    int i, j;
    char far* newname;
    *count = 1;
    name = (unsigned char*)malloc(1024);
    newname = (unsigned char*)malloc(1024);

    name[0] = '\0';

    //read the names in 3www6google3com format
    while (*reader != 0)
    {
        if (*reader >= 192)
        {
            offset = (*reader) * 256 + *(reader + 1) - 49152; //49152 = 11000000 00000000 ;)
            reader = response + offset - 1;
            jumped = 1; //we have jumped to another location so counting wont go up!
        }
        else
        {
            name[p++] = *reader;
        }

        reader = reader + 1;

        if (jumped == 0)
        {
            *count = *count + 1; //if we havent jumped to another location then we can count up
        }
    }

    name[p] = '\0'; //string complete
    if (jumped == 1)
    {
        *count = *count + 1; //number of steps we actually moved forward in the packet
    }

    //now convert 3www6google3com0 to www.google.com
    for (i = 0; i < (int)strlen((const char*)name); i++)
    {
        p = name[i];
        for (j = 0; j < (int)p; j++)
        {
            name[i] = name[i + 1];
            i = i + 1;
        }
        name[i] = '.';
    }
    name[i - 1] = '\0'; //remove the last dot

    for (j = 0; j <= i - 1; j++) newname[j] = name[j];

    return newname;
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