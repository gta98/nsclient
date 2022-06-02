// FileTransferSender.c : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "FileTransferCommon/common.h"

char* remote_addr;// [MAX_DNS_SERVER_ADDR_LENGTH + 1] ;

int socket_send_file(const SOCKET* sock, const char* file_name, uint64_t* file_size, uint64_t* file_total_sent);
int validateHost(const char* hostname);
struct hostent* dnsQuery(const char* hostname);
int fillDnsQueryBuf(const char* hostname, const char* query);
int parseDnsResponseBuf(const char* response, struct hostent* remoteHost);
void printRemoteHost(struct hostent* remoteHost);

int main(const int argc, const char *argv[])
{
    u_short remote_port;
    WSADATA wsaData;
    struct hostent* remoteHost;
    int status;
    DWORD dwError;
    int i;
    
    char file_name[MAX_HOSTNAME_LENGTH_INPUT];
    char hostname[MAX_HOSTNAME_LENGTH_INPUT];
    uint64_t file_size, file_total_sent;

    if (argc != 2) {
        remote_addr = DEFAULT_DNS_SERVER;
        remote_port = DEFAULT_DNS_PORT;
        printd("WARNING: proper syntax is as follows:\n");
        printd("         %s DNS_SERVER\n", argv[0]);
        printd("         an invalid number of arguments was specified, so reverting to DNS_SERVER=%s\n", remote_addr);
    }
    else if (strlen(argv[0]) > MAX_DNS_SERVER_ADDR_LENGTH) {
        /* Not my problem - fail silently */
        remote_addr = "-1";
    }
    else {
        remote_addr = argv[1];
        remote_port = DEFAULT_DNS_PORT; // (u_short)atoi(argv[2]);
    }

    if (socket_initialize(&wsaData) != NO_ERROR) {
        printf(MSG_ERR_WSASTARTUP);
        return 1;
    }

    while (1) {
        printf(MSG_ENTER_HOSTNAME);
#if FLAG_SKIP_FILENAME==1
        strncpy_s(hostname, 100, DEBUG_HOSTNAME, strlen(DEBUG_HOSTNAME));
#else
        scanf_s("%s", hostname, MAX_HOSTNAME_LENGTH_INPUT);
#endif

        status = validateHost(hostname);
        if (status != STATUS_SUCCESS) {
            printf(MSG_ERR_BAD_NAME);
            continue;
        }

        remoteHost = dnsQuery(hostname);

        printRemoteHost(remoteHost);

#if FLAG_SINGLE_ITER==1
        break;
#endif
    }

    return 0;
}

struct hostent* dnsQuery(const char* hostname) {
    int i, j, status;
    SOCKET sock;
    struct hostent* remoteHost;
    char query[SIZE_DNS_QUERY_BUF];
    char response[SIZE_DNS_RESPONSE_BUF];

#if FLAG_IGNORE_SOCKET == 1
    remoteHost = gethostbyname(hostname);
#else
    status = socket_connect(&sock, remote_addr, DEFAULT_DNS_PORT);
    if (status == STATUS_SUCCESS) {
        return NULL;
    }
    remoteHost = NULL;

    fillDnsQueryBuf(hostname, query);
    
    status = sendto(sock, query, SIZE_DNS_QUERY_BUF, 0, NULL, 0);
    if (status != STATUS_SUCCESS) goto dnsQueryFailure;
    
    status = recvfrom(sock, response, SIZE_DNS_RESPONSE_BUF, 0, NULL, 0);
    if (status != STATUS_SUCCESS) goto dnsQueryFailure;

    parseDnsResponseBuf(hostname, response, remoteHost);
#endif

dnsQueryFinish:
    if (sock) closesocket(sock);
    return remoteHost;

dnsQueryFailure:
    remoteHost = NULL;
    goto dnsQueryFinish;
}


int fillDnsQueryBuf(const char* hostname, const char* query) {
    return STATUS_ERR_PLACEHOLDER;
}


int parseDnsResponseBuf(const char* response, struct hostent* remoteHost) {
    return STATUS_ERR_PLACEHOLDER;
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
    for (seg_start=0; (seg_start<length);) {
        for (seg_end = seg_start; (seg_end < length) && (hostname[seg_end]!='.'); seg_end++);
        /* now, we are ensured that hostname[seg_end]=='.' or seg_end==length */
        if (seg_start==seg_end) {
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
    int i;
    i = 0;
    if (remoteHost) {
        if (remoteHost->h_addrtype == AF_INET)
        {
            while (remoteHost->h_addr_list[i] != 0) {
                addr.s_addr = *(u_long*)remoteHost->h_addr_list[i++];
                printf("%s\n", inet_ntoa(addr));
                break;
            }
        }
        return;
    }

    if ((!remoteHost) || (i == 0)) {
        printf(MSG_ERR_NONEXISTENT);
    }
}