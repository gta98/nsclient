#include "socket_utils.h"

boolean socket_initialize(WSADATA* wsaData) {
    return WSAStartup(MAKEWORD(2, 2), wsaData);
}

int socket_listen(SOCKET* sock, SOCKADDR_IN* sock_addr, const uint16_t port) {
    int status;
    SOCKADDR_IN sockaddr;
    char host_name[100];
    gethostname(host_name, sizeof(host_name));
    struct hostent* remotehost = 0;
    struct in_addr addr;


    if (host_name == NULL)
    {
        printf("error initiating gethostname\n");
        strncpy_s(host_name, 8, "0.0.0.0\0", 8);
    }
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if ((*sock) == -1) {
        return STATUS_ERR_SOCK_CREATE;
    }
    remotehost =  gethostbyname(host_name);
    if (remotehost == NULL) {
        printf("ERROR with remotehost name");
    }
    else {
        addr.s_addr = *(u_long*)remotehost->h_addr_list[0];
    }
    sockaddr.sin_addr.s_addr = inet_addr(inet_ntoa(addr)); //this line was derived by other students assignment
    sockaddr.sin_port = htons(port);
    sockaddr.sin_family = AF_INET;
    status = bind(*sock, (SOCKADDR_IN*)&sockaddr, sizeof(sockaddr));
    if (status == -1) {
        return STATUS_ERR_SOCK_BIND;
    }

    if ((listen(*sock, SOCKET_BACKLOG)) < 0) {
        return STATUS_ERR_SOCK_LISTEN;
    }
    char* port_id = malloc(sizeof(char) * 8);
    sprintf_s(port_id, sizeof(char) * 8, "%d", port);
    
    if (port == CHANNEL_PORT_SENDER)
    {
        printf("sender socket: IP Adress =  %s Port = %d\n", inet_ntoa(sockaddr.sin_addr), CHANNEL_PORT_SENDER);
    }
    else if (port == CHANNEL_PORT_RECEIVER)
    {
        printf("receiver socket: IP Adress =  %s Port = %d\n", inet_ntoa(sockaddr.sin_addr), CHANNEL_PORT_RECEIVER);
    }
    return STATUS_SUCCESS;
}

int socket_connect(SOCKET* sock, const char* dest, const u_short port) {
    SOCKADDR_IN sockaddr;
    int status;
    sockaddr.sin_addr.s_addr = inet_addr(dest);
    sockaddr.sin_port = htons(port);
    sockaddr.sin_family = AF_INET;
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    status = connect(*sock, &sockaddr, sizeof(sockaddr));
    
    printd("connection with SERVER failed, error %ld\n", WSAGetLastError());

    return status;
}

int safe_recv(SOCKET* sock, char* buf, int len) {
    char hold[1];
    int left = len;
    int idx;
    int status;
    uint8_t* buf_ack[1]; // FIXME - move to static var
    buf_ack[0] = 0b00000001;

    for (idx = 0; idx < len; idx++) buf[idx] = 0;

    while (left > 0) {
        idx = len - left;
        hold[0] = 0;
        status = recv(sock, hold, 1, 0);
        if (status != -1) {
            buf[idx] = hold[0];
            left -= 1;
        }

        if (status < 0) {
            return STATUS_SOCK_CLOSED;
        }

        send(sock, buf_ack, 1, 0);
        
    }

    return STATUS_SUCCESS;


}

void safe_send(SOCKET* sock, char* buf, int len) {
    char* hold;
    int status;
    uint64_t sent = 0;
    uint8_t* buf_ack[1]; // FIXME - move to static var
    buf_ack[0] = 0b00000001;

    do {
        hold = malloc(sizeof(char));
    } while (hold == NULL);

    while (sent < len) {
        hold[0] = buf[sent];
        status = send(sock, hold, 1, 0);
        if (status > 0) {
            sent += 1;
        }

        while (recv(sock, hold, 1, 0) < 0) {
            continue;
        }
        if (hold[0] != buf_ack[0]) {
            // bad ack
            return STATUS_SOCK_CLOSED;
        }
    }

    return STATUS_SUCCESS;
}