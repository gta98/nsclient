#include "socket_utils.h"

boolean socket_initialize(WSADATA* wsaData) {
    return WSAStartup(MAKEWORD(2, 2), wsaData);
}

int socket_connect(SOCKET* sock, const char* dest, const u_short port) {
    SOCKADDR_IN sockaddr;
    int status;
    int timeout_ms;
    sockaddr.sin_addr.s_addr = inet_addr(dest);
    sockaddr.sin_port = htons(port);
    sockaddr.sin_family = AF_INET;
    *sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    status = connect(*sock, &sockaddr, sizeof(sockaddr));
    timeout_ms = SOCKET_RECV_TIMEOUT_MS;
    //setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
    return status;
}