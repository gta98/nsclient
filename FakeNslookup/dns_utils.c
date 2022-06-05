#include "dns_utils.h"

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


struct hostent* dnsQuery(const char* hostname) {
    struct hostent* remoteHost;
    int i, j, status;
    unsigned char* query;
    char* response;
    size_t sizeof_response;
    size_t* p_sizeof_query, sizeof_query;

    remoteHost = NULL;
    query = NULL;
    response = NULL;
    sizeof_query = 0;
    p_sizeof_query = &sizeof_query;

#if FLAG_IGNORE_SOCKET == 1
    remoteHost = gethostbyname(hostname);
#else

    query = createDnsQueryBuf(hostname, p_sizeof_query);
    if (!query) {
        printd("Could not create DNS query buffer!\n");
        goto dnsQueryFailure;
    }
    //sizeof_query = strlen(query);  //TODO - not the way to check its length!!!!! 
    assert(query);
    for (int i = 0; i < 100; i++)
    {
        printf("%c", query[i]);
    }

    status = sendto(sock, query, sizeof_query, 0, NULL, 0);
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

    remoteHost = parseDnsResponseBuf(response);
    if (!remoteHost) {
        printd("Failure in parsing response from DNS server!\n");
        goto dnsQueryFailure;
    }
#endif

dnsQueryFinish:
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
    char* query;
//    int sizeof_query;
    int sizeof_qname;
    struct dns_header* dns;
    struct question *ques;
    struct question* qinf_struct;
    unsigned char buf[SIZE_DNS_QUERY_BUF], *qname, qinf[SIZE_DNS_QUERY_BUF]; //TODO - maybe change size

    ques = NULL;
    

    // DNS - HEADER //
    dns = (struct dns_header*)&buf;
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


    //  QUESTION //
    qname = (unsigned char*)&qinf;
    sizeof_qname = change_question_name(hostname, qname) +1;


    memcpy(&buf[sizeof(struct dns_header)], qname, sizeof_qname*sizeof(char)+1);
    memset(&qinf, '\0', sizeof(unsigned char) +1);


    ques = (struct question*)&qinf_struct;
    ques->qtype = htons(1);
    ques->qclass = htons(1);


    memcpy(&buf[12 + sizeof_qname], ques, sizeof(struct question));
    for (int i = 0; i < 100; i++)
    {
        printf("%x", buf[i]);
    }
    printf("%d, %d, %d", sizeof(struct dns_header), sizeof(struct question), sizeof(char) * sizeof_qname);
    *sizeof_query = (size_t)(sizeof(struct dns_header) + sizeof(struct question) + sizeof(char) * sizeof_qname);
    printf("%d", *sizeof_query);
    return buf;
    //memcpy(&buffer, buf, sizeof(struct dns_header) + sizeof(struct question) + sizeof(char) * sizeof_qname);
    //for (int i = 0; i < 50; i++)
 //   {
   //     printf("%x", buf
    //}


}

int change_question_name(const char* hostname, unsigned char* qname)
{
    int i, j;
    int sizeof_qname;
    int count;
    char* temp;

    j = 0;
    count = 0;
    temp = NULL;
    sizeof_qname = strlen(hostname) + 1; /* FIXME: Tom - what do we need to reduce sizeof_query to? */
    temp = malloc(sizeof(char) * sizeof_qname); //TODO - free allocation


    for (i = 0; i < sizeof_qname + 1; i++) {
        if (hostname[i] == '.' || hostname[i] == '\0')
        {
            int l = 0;
            sprintf(temp, "%d", count);
            if (strlen(temp) > 9) realloc(qname, sizeof_qname + strlen(temp));
            while (temp[l] != '\0') {
                qname[j] = temp[l];
                j++;
                l++;
            }
            for (int k = i - count; k < i; k++)
            {
                if (isupper(hostname[k])) {
                    qname[j] = tolower(hostname[k]); continue;
                }
                qname[j] = hostname[k];
                j++;
            }
            count = -1;
        }
        count++;
    }
    qname[j] = '\0';
    return j;
    free(temp);
}


struct hostent* parseDnsResponseBuf(const char* response) {
    /*
    * INPUT: "response": fetched from DNS server through recvfrom()
    * RETURN: hostent object with returned IP
    */
    struct hostent* remoteHost;
    size_t sizeof_response;
    int i, j;
    
    remoteHost = malloc(sizeof(struct hostent));
    if (!remoteHost) return NULL;
    sizeof_response = strlen(response);

    /* FIXME: Tom - Implement here - fill remoteHost based on DNS response */

    return remoteHost;
}


int validateHost(const char* hostname) {
    int i, j, seg_start, seg_end;
    size_t length;

    length = strlen(hostname);

    if ((length == 0) || (length > MAX_HOSTNAME_LENGTH)) {
        return STATUS_ERR_BAD_NAME;
    }

    if ((hostname[0] == '-') || (hostname[length - 1] == '-')) {
        return STATUS_ERR_BAD_NAME;
    }

    if (hostname[length - 1] == '.') {
        /* apparently, hostname can end with a dot */
        length--;
        if (length < 0) {
            return STATUS_ERR_BAD_NAME;
        }
    }

    /* each segment must have 1<=n<=63 characters */
    for (seg_start = 0; (seg_start < length);) {
        for (seg_end = seg_start; (seg_end < length) && (hostname[seg_end] != '.'); seg_end++);
        /* now, we are ensured that hostname[seg_end]=='.' or seg_end==length */
        if (seg_start == seg_end) {
            return STATUS_ERR_BAD_NAME;
        }
        if ((hostname[seg_start] == '-') || (hostname[seg_end - 1] == '-')) {
            return STATUS_ERR_BAD_NAME;
        }
        for (i = seg_start; i < seg_end; i++) {
            if ((hostname[i] == '-')
                || (('0' <= hostname[i]) && (hostname[i] <= '9'))
                || (('a' <= hostname[i]) && (hostname[i] <= 'z'))
                || (('A' <= hostname[i]) && (hostname[i] <= 'Z'))) {
                /* all good */
            }
            else {
                return STATUS_ERR_BAD_NAME;
            }
        }
        seg_start = seg_end + 1;
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