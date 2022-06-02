#include "dns_utils.h"

struct hostent* dnsQuery(const char* hostname) {
    struct hostent* remoteHost;
    int i, j, status;
    char query[SIZE_DNS_QUERY_BUF];
    char response[SIZE_DNS_RESPONSE_BUF];
    int sizeof_query, sizeof_response;

    sock = NULL;
    remoteHost = NULL;

#if FLAG_IGNORE_SOCKET == 1
    remoteHost = gethostbyname(hostname);
#else
    status = createDnsQueryBuf(hostname, query);
    if (status != STATUS_SUCCESS) {
        printd("Could not create DNS query buffer - status %d\n", status);
        goto dnsQueryFailure;
    }

    sizeof_query = strlen(query); /* note: must be terminated by 0 */
    status = sendto(sock, query, sizeof_query, 0, NULL, 0);
    if (status == SOCKET_ERROR) {
        printd("Could not send query to DNS server = status %d\n", status);
        goto dnsQueryFailure;
    }

    sizeof_response = sizeof(response); /* longest permitted response */
    status = recvfrom(sock, response, sizeof_response, 0, NULL, 0);
    if (status == SOCKET_ERROR) {
        printd("Failure in receiving response from DNS server - status %d\n", status);
        goto dnsQueryFailure;
    }
    sizeof_response = status; /* actual response size */

    remoteHost = malloc(sizeof(struct hostent));
    if (!remoteHost) {
        printd("Failure malloc'ing remoteHost!\n");
        goto dnsQueryFailure;
    }

    status = parseDnsResponseBuf(response, remoteHost);
    if (status != STATUS_SUCCESS) {
        printd("Failure in parsing response from DNS server - status %d\n", status);
        goto dnsQueryFailure;
    }
#endif

dnsQueryFinish:
    return remoteHost;

dnsQueryFailure:
    if (remoteHost) {
        free(remoteHost);
        remoteHost = NULL;
    }
    goto dnsQueryFinish;
}


int createDnsQueryBuf(const char* hostname, char query[]) {
    /*
    * INPUT: "hostname": e.g. "google.com", "www.ynet.co.il"
    * INPUT: "query[]": already allocated, with sizeof SIZE_DNS_QUERY_BUF
    * OUTPUT: "query[]":
    *         This string is sent to the DNS server
    *         It contains the request "give me the IP address for <hostname>"
    * RETURN: STATUS_SUCCESS or STATUS_ERR_*
    */
    int i, j;

    for (i = 0; i < SIZE_DNS_QUERY_BUF; i++) query[i] = 0;

    /* FIXME: Implement here */

    return STATUS_ERR_NOT_IMPLEMENTED;
}


int parseDnsResponseBuf(const char response[], const int sizeof_response, struct hostent* remoteHost) {
    /*
    * INPUT: "response[]": fetched from DNS server through recvfrom()
    * INPUT: "sizeof_response": size of actual contents of response[], returned by recvfrom
    * INPUT: "remoteHost": pointer to hostent object, which we would write into
    * OUTPUT: "remoteHost"
    * RETURN: STATUS_SUCCESS or STATUS_ERR_*
    */
    int i, j;

    /* FIXME: Needs implementation */

    return STATUS_ERR_NOT_IMPLEMENTED;
}


int validateHost(const char* hostname) {
    int length, i, j, seg_start, seg_end;

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