#include "dns_utils.h"


struct hostent* dnsQuery(const char* hostname) {
    struct hostent* remoteHost;
    int i, j, status;
    char* query;
    char* response;
    size_t sizeof_query, sizeof_response;

    remoteHost = NULL;
    query = NULL;
    response = NULL;

#if FLAG_IGNORE_SOCKET == 1
    remoteHost = gethostbyname(hostname);
#else
    query = createDnsQueryBuf(hostname);
    if (!query) {
        printd("Could not create DNS query buffer!\n");
        goto dnsQueryFailure;
    }
    sizeof_query = strlen(query);
    assert(sizeof_query > 0);

    status = sendto(sock, query, sizeof_query, 0, NULL, 0);
    if (status == SOCKET_ERROR) {
        printd("Could not send query to DNS server due to socket error\n");
        goto dnsQueryFailure;
    }

    sizeof_response = SIZE_DNS_RESPONSE_BUF;
    response = malloc(sizeof(char) * (sizeof_response + 1));
    if (!response) {
        printd("Could not allocate DNS response buffer!\n");
        goto dnsQueryFailure;
    }
    status = recvfrom(sock, response, sizeof_response, 0, NULL, 0);
    if (status == SOCKET_ERROR) {
        printd("Failure in receiving response from DNS server - status %d\n", status);
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


char* createDnsQueryBuf(const char* hostname) {
    /*
    * INPUT: "hostname": e.g. "google.com", "www.ynet.co.il"
    * RETURN: "query[]":
    *         This string is sent to the DNS server
    *         It contains the request "give me the IP address for <hostname>"
    */
    char* query;
    int i, j;
    int sizeof_query;

    sizeof_query = SIZE_DNS_QUERY_BUF; /* FIXME: Tom - what do we need to reduce sizeof_query to? */
    query = malloc(sizeof(char)*(sizeof_query+1));
    if (!query) return NULL;
    for (i = 0; i < sizeof_query+1; i++) query[i] = 0;

    /* FIXME: Tom - Implement here - fill query[0 to sizeof_query] based on hostname */

    return query;
}


struct hostent* parseDnsResponseBuf(const char* response) {
    /*
    * INPUT: "response": fetched from DNS server through recvfrom()
    * RETURN: hostent object with returned IP
    */
    struct hostent* remoteHost;
    int sizeof_response;
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