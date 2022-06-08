#include "dns_utils.h"


struct hostent* dnsQuery(const char* hostname) {
    /*
    * INPUT: "hostname": e.g. "google.com", "www.ynet.co.il"
    * RETURN: a hostent object == gethostbyname(hostname)
    */
    struct hostent* remoteHost;
    int status;
    unsigned char* query;
    unsigned char* response;
    size_t sizeof_query, sizeof_response, sizeof_qname;
  //  answer_t* ans;

    remoteHost = NULL;
    query = NULL;
    response = NULL;
    sizeof_query = 0;
    sizeof_qname = 0;

#if FLAG_REAL_NSLOOKUP == 1
    remoteHost = gethostbyname(hostname);
#else

    query = createDnsQueryBuf(hostname, &sizeof_query,&sizeof_qname);
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

    remoteHost = parseDnsResponseBuf(response, sizeof_response,sizeof_qname);
    
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





char* createDnsQueryBuf(const char* hostname, size_t* sizeof_query, size_t* sizeof_qname) {
    /*
    * INPUT: "hostname": e.g. "google.com", "www.ynet.co.il"
    * RETURN: "query[]":
    *         This string is sent to the DNS server
    *         It contains the request "give me the IP address for <hostname>"
    */
    char* buf;
    dns_header_t *dns;
   // size_t sizeof_qname;
    char* qname;
    question_t *qinf;

    *sizeof_query = sizeof(dns_header_t) + (sizeof(char) * 1024) + sizeof(question_t);
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
    *sizeof_qname = change_question_name(hostname, qname);
    *sizeof_query = (*sizeof_query) - (sizeof(char) * 1024) + *sizeof_qname;

    qinf = (question_t*)(buf + sizeof(dns_header_t) + *sizeof_qname);
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


struct hostent* parseDnsResponseBuf(const unsigned char* response, size_t sizeof_response, size_t sizeof_qname) {
    /*
    * INPUT: "response": fetched from DNS server through recvfrom()
    *        sizeof_response
    *        sizeof_qname : size of the name **sent** to DNS
    * RETURN: hostent object with returned IP
    */
    struct hostent* remoteHost;
    const unsigned char* reader;
    dns_header_t* dns;
    question_t* ques;
    int i;
    int status;
    char* aliases;
    char* h_addr_list;
    char** h_addr_list_ptr;
    char** aliases_ptr;
    struct in_addr* addr_s;


    h_addr_list_ptr = (char**)malloc(sizeof(char**));
    if (!h_addr_list_ptr) {
        return NULL;
    }
    aliases_ptr = (char**)malloc(sizeof(char**));
    if (!aliases_ptr) {
        free(h_addr_list_ptr);
        return NULL;
    }
    

    printd("About to print response\n");
    printAsBytes(response, sizeof_response);
    printd("About to parse remoteHost from response\n");
 
    remoteHost = malloc(sizeof(struct hostent));
    if (!remoteHost) {
        free(aliases_ptr);
        free(h_addr_list_ptr);
        return NULL;
    }

    reader = response + 0;

    dns = (dns_header_t*)reader;
    parseDnsHeaderFromResponse(dns);
    if ((dns->rcode != 0) || (dns->ans_count < 1)) {
        free(aliases_ptr);
        free(h_addr_list_ptr);
        free(remoteHost);
        return NULL;
    }

    aliases = NULL;
    memcpy(aliases_ptr, &aliases, sizeof(char**));
    remoteHost->h_aliases = aliases_ptr;

    reader += sizeof(dns_header_t);
    printAsBytes(reader, sizeof(response_t));

    reader += sizeof_qname;
    ques = (question_t*)reader;
    reader += sizeof(question_t);

    // here reader is in position of ANSWER //

    status = read_qname_wrapper(reader, sizeof_qname, sizeof_response, response, remoteHost);
    if (status != STATUS_SUCCESS) {
        free(aliases_ptr);
        free(h_addr_list_ptr);
        free(remoteHost);
        return NULL;
    }
    reader += 2 * sizeof(char);

    uint16_t rtype = (reader[0] << 4) | (reader[1]);
    reader += 2 * sizeof(char);
    uint16_t rclass = (reader[0] << 4) | (reader[1]);
    reader += 2 * sizeof(char);
    uint32_t rttl = (reader[0] << 12) | (reader[1] << 8) | (reader[2] << 4) | (reader[3] << 0);
    reader += 4 * sizeof(char);
    uint16_t rdlength = (reader[0] << 4) | (reader[1]);
    reader += 2 * sizeof(char);
    remoteHost->h_length =rdlength;
    remoteHost->h_addrtype = AF_INET; //rtype;

    if (remoteHost->h_addrtype == 5) reader += (remoteHost->h_length)*sizeof(char);
    addr_s = (struct in_addr*)reader;
    
    size_t h_addr_list_len = (response + sizeof_response) - reader;
    h_addr_list = malloc(h_addr_list_len);
    if (!h_addr_list) {
        free(aliases_ptr);
        free(h_addr_list_ptr);
        free(remoteHost->h_name);
        free(remoteHost);
        return NULL;
    }
    memcpy(h_addr_list_ptr, &h_addr_list, sizeof(char**));
    //h_addr_list_ptr = &h_addr_list;
    for (i = 0; i < h_addr_list_len; i++) h_addr_list[i] = reader[i];
    remoteHost->h_addr_list = h_addr_list_ptr;
    assertd(addr_s->S_un.S_addr == *(u_long*)remoteHost->h_addr_list[0]);


    return remoteHost;
}


int read_qname_wrapper(const unsigned char* reader, size_t sizeof_qname, size_t sizeof_response,
                        const unsigned char* response, struct hostent* remoteHost) {
    // returns STATUS_SUCCESS if error, else STATUS_ERROR
    uint16_t rname_lbl_ptr;
    char* name_frm_ptr;
    int rname_offset, k, j, addition;
    rname_lbl_ptr = (reader[0] >> 6); // specify label of domain or pointer to name in response
    name_frm_ptr = (char*)malloc(sizeof_qname + 1);
    if (name_frm_ptr == NULL) {
        return STATUS_ERROR;
    }
    if (rname_lbl_ptr == 3) // if pointer to lable:
    {
        rname_offset = reader[0];
        k = 0;
        j = 0;
        rname_offset = removeSignificantBit(rname_offset);
        rname_offset = removeSignificantBit(rname_offset);
        addition = reader[1];
        rname_offset = rname_offset << 2;
        rname_offset += addition;
        for (j = 0; j <= sizeof_qname; j++) {
            if ((rname_offset >= sizeof_response) || (rname_offset < 0)) {
                // in this case, the DNS server is telling us to perform access violation
                free(name_frm_ptr);
                return STATUS_ERROR;
            }
#pragma warning( push )
#pragma warning( disable : 6386 )
            // doesn't seem to be a real issue here, so I suppressed it
            name_frm_ptr[j] = response[rname_offset];
#pragma warning( pop )
            rname_offset++;
        }
    }
    else if (rname_lbl_ptr == 0)
    {
        for (int j = 0; j <= sizeof_qname; j++) {
#pragma warning( push )
#pragma warning( disable : 6386 )
            // doesn't seem to be a real issue here, so I suppressed it
            name_frm_ptr[j] = response[sizeof(dns_header_t)+j];
#pragma warning( pop )
        }
    }
    read_qname(name_frm_ptr, &remoteHost->h_name);
    free(name_frm_ptr);
    return STATUS_SUCCESS;
}

size_t read_qname(const unsigned char* reader, char far** h_name) {
    /*
    * INPUT: "reader": pointer to location in which we can expect the hostname in the response
    *                  we expect 3www6google3com0 style formatting
    * INPUT: "h_name": pointer to pointer in which we want to store the extracted hostname
    * OUTPUT: "h_name": will be in www.google.com format
    * RETURN: size of name as stored within reader (3www6google3com0 -> 16) 
    */
    int i;
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
    printd("QUESTIONS=%d, ANSWERS=%d, AUTH=%d, ADDITIONAL=%d",
        ntohs(dns->q_count), ntohs(dns->ans_count), ntohs(dns->auth_count), ntohs(dns->add_count));

    switch (dns->rcode) {
    case 0:
        break;
    case 1: printd("Badly formatted domain\n");
        break;
    case 2: printd("Server failure\n");
        break;
    case 3: printd("Domain does not exist\n");
        break;
    case 4: printd("Unsupported query type\n");
        break;
    case 5: printd("Query refused by server\n");
        break;
    default: printd("Reserved\n");
        break;
    }
}


int validateHost(const unsigned char* _hostname) {
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
            goto validate_hostname_bad_name;
        }
        /* RFC 1035 states:
            <label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]
        */
        if (!is_letter(label[0])) {
            goto validate_hostname_bad_name;
        }
        if (!str_check_all(label, is_let_dig_hyp)) {
            goto validate_hostname_bad_name;
        }
        if (!is_let_dig(label[label_length - 1])) {
            goto validate_hostname_bad_name;
        }
        label = strtok(NULL, ".");
    }

    for (i = 0; i < length-1; i++) {
        if ((_hostname[i] == '.') && (_hostname[i + 1] == '.')) {
            goto validate_hostname_bad_name;
        }
    }

    free(hostname);
    return STATUS_SUCCESS;

    validate_hostname_bad_name:
    free(hostname);
    return STATUS_ERR_BAD_NAME;
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
        for (i = 0; (remoteHost->h_aliases[i] || remoteHostDebug->h_aliases[i]); i++) {
            assertd(remoteHost->h_aliases[i] && remoteHostDebug->h_aliases[i]);
            assertd(strcmp(remoteHost->h_aliases[i], remoteHostDebug->h_aliases[i]) == 0);
        }
        for (i = 0; (remoteHost->h_addr_list[i] || remoteHostDebug->h_addr_list[i]); i++) {
            assertd(remoteHost->h_addr_list[i] && remoteHostDebug->h_addr_list[i]);
            assertd(strcmp(remoteHost->h_addr_list[i], remoteHostDebug->h_addr_list[i]) == 0);
        }
    }
#else
    return;
#endif
}