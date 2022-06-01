#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define CHANNEL_ADDR "127.0.0.1"
#define CHANNEL_PORT_SENDER   46342
#define CHANNEL_PORT_RECEIVER 46343

#define STATUS_SUCCESS             0
#define STATUS_ERR_FILE_READ       1
#define STATUS_ERR_MALLOC_BUF      2
#define STATUS_ERR_MALLOC_BUF_ENC  3
#define STATUS_ERR_BUF_SIZE        4
#define STATUS_ERR_CORRUPT_ADDED   5
#define STATUS_ERR_CORRUPT_SIZE    6
#define STATUS_ERR_SOCK_CREATE     7
#define STATUS_ERR_SOCK_BIND       8
#define STATUS_ERR_SOCK_LISTEN     9
#define STATUS_ERR_SOCK_CONNECT   10
#define STATUS_SOCK_CLOSED        11


#define MSG_ERR_WSASTARTUP     "ERROR: WSAStartup() failed\n"
#define MSG_ERR_CONNECTING     "ERROR: Code=(%d), in connecting to %s:%d\n"
#define MSG_ERR_CREATE_SOCK    "ERROR: Could not create new socket\n"
#define MSG_ERR_SOCK_LISTEN    "ERROR: Could not listen on port %d\n"
#define MSG_ERR_SOCK_BIND      "ERROR: Could not bind to port %d\n"
#define MSG_SUCCESS_LISTEN     "Listening on port %d\n"
#define MSG_ENTER_FILENAME     "Enter filename: "
#define MSG_FILE_LENGTH        "File length: %llu bytes\n"
#define MSG_TOTAL_SENT         "Total sent:  %llu bytes\n"
#define MSG_ERR_FILE_READ      "ERROR: Could not read file %s\n"
#define MSG_ERR_MALLOC_BUF     "ERROR: Could not allocate %llu bytes of memory required for storing file content\n"
#define MSG_ERR_MALLOC_BUF_ENC "ERROR: Could not allocate %llu bytes of memory required for storing encoded file content\n"
#define MSG_ERR_CORRUPT_SIZE   "ERROR: Corrupt expected transmission size, cannot continue\n"
#define MSG_ERR_CORRUPT_ADDED  "ERROR: Corrupt number of zeros received, cannot continue\n"
#define MSG_ERR_BUF_SIZE       "ERROR: Weird buffer size - this should never happen!!!\n"
#define MSG_ERR_UNKNOWN        "ERROR: Unhandled error code (%d)\n"

#define MAX_FILE_PATH_LENGTH           32767
#define MAX_PERMITTED_FILE_PATH_LENGTH 4096


#define SOCKET_BACKLOG 5