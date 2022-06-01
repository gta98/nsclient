// FileTransferSender.c : This file contains the 'main' function. Program execution begins and ends there.
//

#include "FileTransferCommon/common.h"

int socket_recv_file(const SOCKET* sock, const char* file_name, uint64_t* file_size, uint64_t* file_total_recv) {
    uint8_t buf_send[4], buf_hold[1], buf_encode[4], buf_read[4], buf_recv_enc[31], buf_recv_dec[26];
    uint8_t* buf_recv_raw;
    int status;
    errno_t err;
    FILE* fp;
    uint64_t transmission_size;
    int total_zeros_added, total_bytes_not_recv, number_of_zeros;

    if (fopen_s(&fp, file_name, "wb") != 0) {
        return STATUS_ERR_FILE_READ;
    }

    *file_size = 0;
    *file_total_recv = 0;


    safe_recv(sock, buf_recv_enc, 31);
    decode_31_block_to_26(buf_recv_dec, buf_recv_enc);

    total_zeros_added = buf_recv_dec[0];

    transmission_size  = 0;
    transmission_size |= (uint64_t)buf_recv_dec[1] << 56;
    transmission_size |= (uint64_t)buf_recv_dec[2] << 48;
    transmission_size |= (uint64_t)buf_recv_dec[3] << 40;
    transmission_size |= (uint64_t)buf_recv_dec[4] << 32;
    transmission_size |= (uint64_t)buf_recv_dec[5] << 24;
    transmission_size |= (uint64_t)buf_recv_dec[6] << 16;
    transmission_size |= (uint64_t)buf_recv_dec[7] <<  8;
    transmission_size |= (uint64_t)buf_recv_dec[8] <<  0;
    *file_total_recv = transmission_size;

    if ((transmission_size % 31) != 0) {
        return STATUS_ERR_CORRUPT_SIZE;
    }

    uint64_t m = floor(transmission_size / (uint64_t)31);
    *file_size = ((m-1)*26) - total_zeros_added;

    total_bytes_not_recv = transmission_size - 31;
    m--;

    if (total_zeros_added > 25) {
        return STATUS_ERR_CORRUPT_ADDED;
        // alternatively: change total_zeros_added to 0...
    }

    for (int i = 0; i < 26; i++) printd("?? ");
    printd("\n");

    while (m > 1) {
        safe_recv(sock, buf_recv_enc, 31);
        decode_31_block_to_26(buf_recv_dec, buf_recv_enc);
        for (int i = 0; i < 26; i++) {
            buf_hold[0] = buf_recv_dec[i];
            fprintf(fp, "%c", buf_hold[0]);
            printd("%02x ", buf_hold[0]);
        }
        printd("\n");
        total_bytes_not_recv -= 31;
        m--;
    }
    printd("\nnow adding end\n");
    if (m == 1) {
        safe_recv(sock, buf_recv_enc, 31);
        decode_31_block_to_26(buf_recv_dec, buf_recv_enc);
        for (int i = 0; i < (26 - total_zeros_added); i++) {
            buf_hold[0] = buf_recv_dec[i];
            fprintf(fp, "%c", buf_hold[0]);
            printd("%02x ", buf_hold[0]);
        }
        m--;
    }
    printd("\n");
    fclose(fp);
    return 0;
}

int main(const int argc, const char* argv[])
{
    char* remote_addr;
    u_short remote_port;
    SOCKET sock;
    WSADATA wsaData;
    int status;

    char file_name[MAX_PERMITTED_FILE_PATH_LENGTH];
    uint64_t file_size, file_total_recv;

    if (argc != 3) {
        remote_addr = CHANNEL_ADDR;
        remote_port = CHANNEL_PORT_RECEIVER;
        printd("WARNING: proper syntax is as follows:\n");
        printd("         %s IP PORT\n", argv[0]);
        printd("         an invalid number of arguments was specified, so using IP=%s, PORT=%d\n", remote_addr, remote_port);
    }
    else {
        remote_addr = argv[1];
        remote_port = (u_short)atoi(argv[2]);
    }

    if (socket_initialize(&wsaData) != NO_ERROR) {
        printf(MSG_ERR_WSASTARTUP);
        return 1;
    }

    printd("Attempting connection to %s:%d\n", remote_addr, remote_port);
    status = socket_connect(&sock, remote_addr, remote_port);
    if (status == SOCKET_ERROR) {
        printf(MSG_ERR_CONNECTING, status, remote_addr, remote_port);
#if FLAG_IGNORE_SOCKET != 1
        return 1;
#endif
    }
    printd("Connected to %s:%d\n", remote_addr, remote_port);


    while (1) {
        printf(MSG_ENTER_FILENAME);
#if FLAG_SKIP_FILENAME==1
        strncpy_s(file_name, 100, DEBUG_FILE_PATH_RECV, strlen(DEBUG_FILE_PATH_RECV));
#else
        scanf_s("%s", file_name, MAX_PERMITTED_FILE_PATH_LENGTH);
        if (strcmp(file_name, "quit") == 0) {
            return 0;
        }
#endif

        printd("Receiving file...\n");
        status = socket_recv_file(sock, file_name, &file_size, &file_total_recv);
        switch (status) {
        case STATUS_SUCCESS: {
            printf(MSG_FILE_LENGTH, file_size);
            printf(MSG_TOTAL_SENT, file_total_recv);
            break;
        }
        case STATUS_ERR_FILE_READ: {
            printf(MSG_ERR_FILE_READ, file_name);
            break;
        }
        case STATUS_ERR_MALLOC_BUF: {
            printf(MSG_ERR_MALLOC_BUF, file_size);
            break;
        }
        case STATUS_ERR_MALLOC_BUF_ENC: {
            printf(MSG_ERR_MALLOC_BUF_ENC, file_total_recv);
            break;
        }
        case STATUS_ERR_BUF_SIZE: {
            printf(MSG_ERR_BUF_SIZE);
            break;
        }
        case STATUS_ERR_CORRUPT_SIZE: {
            printf(MSG_ERR_CORRUPT_SIZE);
            break;
        }
        case STATUS_ERR_CORRUPT_ADDED: {
            printf(MSG_ERR_CORRUPT_ADDED);
            break;
        }
        default: {
            printf(MSG_ERR_UNKNOWN, status);
            break;
        }
        }
#if FLAG_SINGLE_ITER==1
        break;
#endif
    }

    return 0;
}