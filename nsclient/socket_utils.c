#include "socket_utils.h"

int socket_initialize(WSADATA* wsaData) {
    return WSAStartup(MAKEWORD(2, 2), wsaData);
}

int socket_connect(SOCKET* sock, const char* dest, const u_short port) {
    SOCKADDR_IN _sockaddr;
    int status;
    int timeout_ms;
    _sockaddr.sin_addr.s_addr = inet_addr(dest);
    _sockaddr.sin_port = htons(port);
    _sockaddr.sin_family = AF_INET;
    timeout_ms = SOCKET_RECV_TIMEOUT_MS;
    *sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    status = connect(*sock, (struct sockaddr*)&(_sockaddr), sizeof(_sockaddr));
    setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
    return status;
}