// main.c : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "common.h"


int main(const int argc, const char *argv[])
{
    WSADATA wsaData;
    const char* remote_addr;
    struct hostent* remoteHost;
    int i, status;
    char hostname[MAX_HOSTNAME_LENGTH_INPUT];
#if FLAG_DEBUG == 1
    _CrtMemState state;
#endif

#if FLAG_DEBUG == 1
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtMemCheckpoint(&state);
#endif

#if FLAG_REVERT_DNS_IF_INVALID == 1
    if (argc != 2) {
        remote_addr = DEFAULT_DNS_SERVER;
        printf("WARNING: proper syntax is as follows:\n");
        printf("         %s DNS_SERVER\n", argv[0]);
        printf("         an invalid number of arguments was specified, so reverting to DNS_SERVER=%s\n", remote_addr);
    }
    else if (strlen(argv[0]) > MAX_DNS_SERVER_ADDR_LENGTH) {
        remote_addr = DEFAULT_DNS_SERVER;
        printf("WARNING: invalid DNS server specified - reverting to %s\n", remote_addr);
    }
#else
    if (argc != 2) {
        perror("ERROR: proper syntax is as follows:\n");
        perror("       %s DNS_SERVER\n", argv[0]);
        return 1;
    }
    else if (strlen(argv[0]) > MAX_DNS_SERVER_ADDR_LENGTH) {
        perror("ERROR: Invalid DNS server specified\n");
        return 1;
    }
#endif
    else {
        remote_addr = argv[1];
    }

    printd("Using DNS server %s\n", remote_addr);

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
#if FLAG_SKIP_HOSTNAME==1
        strncpy_s(hostname, 100, DEBUG_HOSTNAME, strlen(DEBUG_HOSTNAME));
        printf("%s\n", DEBUG_HOSTNAME);
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
        assertDnsQueryResultIsValid(remoteHost, hostname);

        if (remoteHost) free(remoteHost);

#if FLAG_SKIP_HOSTNAME==1
        break;
#endif
    }

    if (sock) closesocket(sock);
#if FLAG_DEBUG == 1
    _CrtMemDumpAllObjectsSince(&state);
#endif
    return 0;
}
