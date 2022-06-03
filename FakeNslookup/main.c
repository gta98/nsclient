// main.c : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "common.h"

SOCKET sock;

int main(const int argc, const char *argv[])
{
    WSADATA wsaData;
    char* remote_addr;
    struct hostent* remoteHost;
    int status;
    int i;
    
    char hostname[MAX_HOSTNAME_LENGTH_INPUT];
    
    sock = NULL;

    if (argc != 2) {
        remote_addr = DEFAULT_DNS_SERVER;
        printf("WARNING: proper syntax is as follows:\n");
        printf("         %s DNS_SERVER\n", argv[0]);
        printf("         an invalid number of arguments was specified, so reverting to DNS_SERVER=%s\n", remote_addr);
    }
    else if (strlen(argv[0]) > MAX_DNS_SERVER_ADDR_LENGTH) {
        /* Not my problem - fail silently */
        remote_addr = DEFAULT_DNS_SERVER;
        printf("WARNING: invalid DNS server specified - reverting to %s\n", remote_addr);
    }
    else {
        remote_addr = argv[1];
    }

    status = socket_initialize(&wsaData);
    if (status != NO_ERROR) {
        printf(MSG_ERR_WSASTARTUP);
        return 1;
    }

    status = socket_connect(&sock, remote_addr, DEFAULT_DNS_PORT);
    if (status != STATUS_SUCCESS) {
        printf("ERROR: Could not connect to DNS server at %s:%d\n", remote_addr, DEFAULT_DNS_PORT);
        return 1;
    }

    while (1) {
        printf(MSG_ENTER_HOSTNAME);
#if FLAG_SKIP_FILENAME==1
        strncpy_s(hostname, 100, DEBUG_HOSTNAME, strlen(DEBUG_HOSTNAME));
#else
        status = scanf_s("%s", hostname, MAX_HOSTNAME_LENGTH_INPUT);
        hostname[MAX_HOSTNAME_LENGTH_INPUT - 1] = 0;
        for (i = 0; hostname[i]; i++)
            hostname[i] = tolower(hostname[i]);
#endif

        if (!strcmp(hostname, QUIT_COMMAND_STRING)) {
            break;
        }

        status = validateHost(hostname);
        if (status != STATUS_SUCCESS) {
            printf(MSG_ERR_BAD_NAME);
            continue;
        }

        remoteHost = dnsQuery(hostname);

        printRemoteHost(remoteHost);

        if (remoteHost) free(remoteHost);

#if FLAG_SINGLE_ITER==1
        break;
#endif
    }

    if (sock) closesocket(sock);
    return 0;
}
