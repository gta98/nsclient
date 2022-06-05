#include "dns_utils.h"


struct hostent* dnsQuery(const char* hostname) {
    struct hostent* remoteHost;
    int i, j, status;
    unsigned char* query;
    unsigned char* response;
    size_t sizeof_response;
    size_t* p_sizeof_query, sizeof_query;

    remoteHost = NULL;
    query = NULL;
    response = NULL;
    sizeof_query = 0;

#if FLAG_IGNORE_SOCKET == 1
    remoteHost = gethostbyname(hostname);
#else

    query = createDnsQueryBuf(hostname, &sizeof_query);
    if (!query) {
        printd("Could not create DNS query buffer!\n");
        goto dnsQueryFailure;
    }
    assert(sizeof_query > 0);
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
    size_t sizeof_qname;
    dns_header_t *dns;
    question_t *ques;
    question_t *qinf_struct;
    unsigned char *qname, *qinf; //TODO - maybe change size

    *sizeof_query = sizeof(dns_header_t) + (sizeof(char) * 1024) + sizeof(question_t); // FIXME - set constant SIZE_DNS_QUERY_BUF;
    buf = NULL;
    buf = malloc(sizeof(char) * (*sizeof_query));
    if (!buf) {
        return NULL;
    }

    dns = (dns_header_t*)(buf + 0);
    dns->id = (unsigned short)htons(getpid());
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

    qname = (unsigned char*) (buf + sizeof(dns_header_t));
    sizeof_qname = (sizeof(char))*change_question_name(hostname, qname);
    *sizeof_query = (*sizeof_query) - (sizeof(char) * 1024) + sizeof_qname;

    qinf_struct = (question_t*)(buf + sizeof(dns_header_t) + sizeof_qname);
    qinf_struct->qtype = htons(1);
    qinf_struct->qclass = htons(1);

    printf("Final size is %zu\n", *sizeof_query);

    return buf;

}

int change_question_name(const unsigned char* hostname, unsigned char* dst) {
    int i, j, count_dots, len_original, len_new, seg_start, seg_end;
    int dst_counter;
    int seg_len;
    char* token;

    len_original = strlen(hostname);
    dst_counter = 0;
    token = strtok(hostname, ".");
    while (token != NULL) {
        seg_len = strlen(token);
        assert((1 <= seg_len) && (seg_len <= 63)); /* Already verified earlier */
        dst[dst_counter] = (unsigned char) seg_len;
        dst_counter += 1;
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

    return dst_counter;
}


struct hostent* parseDnsResponseBuf(const unsigned char* response, size_t sizeof_response) {
    /*
    * INPUT: "response": fetched from DNS server through recvfrom()
    * RETURN: hostent object with returned IP
    */
    struct hostent* remoteHost;
    int i, j;
    
    remoteHost = malloc(sizeof(struct hostent));
    if (!remoteHost) return NULL;

    /* FIXME: Tom - Implement here - fill remoteHost based on DNS response */

    return remoteHost;
}


int validateHost(const unsigned char* hostname) {
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
    int i, j, seg_start, seg_end;
    int label_length;
    char* label;
    size_t length;

    length = strlen(hostname);

    if ((length == 0) || (length > 255)) {
        /* RFC 1035 states:
            the total length of a domain name (i.e., label octets and label length octets)
            is restricted to 255 octets or less.
        */
        return STATUS_ERR_BAD_NAME;
    }

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
    if (remoteHostDebug != NULL) assert(remoteHost != NULL);
    if (remoteHost != NULL) assert(remoteHostDebug != NULL);
    if (remoteHost && remoteHostDebug) {
        assert(strcmp(remoteHost->h_name, remoteHostDebug->h_name) == 0);
        assert(remoteHost->h_length == remoteHostDebug->h_length);
        assert(remoteHost->h_addrtype == remoteHostDebug->h_addrtype);
        for (i = 0; (remoteHost->h_addr_list[i] || remoteHostDebug->h_addr_list[i]); i++) {
            assert(remoteHost->h_addr_list[i] && remoteHostDebug->h_addr_list[i]);
            assert(strcmp(remoteHost->h_addr_list[i], remoteHostDebug->h_addr_list[i]) == 0);
        }
        for (i = 0; (remoteHost->h_aliases[i] || remoteHostDebug->h_aliases[i]); i++) {
            assert(remoteHost->h_aliases[i] && remoteHostDebug->h_aliases[i]);
            assert(strcmp(remoteHost->h_aliases[i], remoteHostDebug->h_aliases[i]) == 0);
        }
    }
#else
    return;
#endif
}