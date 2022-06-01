// FileTransferSender.c : This file contains the 'main' function. Program execution begins and ends there.
//

#include "FileTransferCommon/common.h"

int socket_send_file(const SOCKET* sock, const char* file_name, uint64_t* file_size, uint64_t* file_total_sent) {
    unsigned char buf_send[4], buf_hold[1], buf_encode[4], buf_read[4], buf_send_enc[31];
    uint8_t buf_raw[26], buf_enc[31];
    uint8_t buf_enc_tmp[31];
    uint64_t file_size_left;
    errno_t err;
    FILE* fp;

    *file_size       = 0;
    *file_total_sent = 0;

    if (fopen_s(&fp, file_name, "rb") != 0) {
        return STATUS_ERR_FILE_READ;
    }

    fseek(fp, 0, SEEK_END);
    *file_size = ftell(fp); // bytes
    // first byte will tell us how many bytes were added to the actual data
    // next 8 bytes will tell us the size of the transmission
    fseek(fp, 0, SEEK_SET);

    uint64_t bytes_left_to_read = *file_size;
    uint8_t bytes_missing_for_26 = (26 - ((*file_size) % 26)) % 26;
    //if (((*file_size) % 26) == 0) bytes_missing_for_26 = 0;
    //else                          bytes_missing_for_26 = 26 - ((*file_size) % 26);
    //int bytes_missing_for_26 = (26 - (1+8+(*(file_size)) % 26)) % 26;
    printd("bytes_missing_for_26=%d\n", bytes_missing_for_26);
    int total_zeros_added = bytes_missing_for_26;
    printd("total zeros added: %d\n", total_zeros_added);
    int buf_size = 26 + ((*file_size) + total_zeros_added);
    
    // at this point, we know what the transmission size will be
    // raw=26m bytes = 8*26m bits, so m=raw.bytes/26=buf_size/26, encoded=8*31m bits = 31m bytes
    //if (floor(buf_size / 26) != ceil(buf_size / 26)) return STATUS_ERR_BUF_SIZE;
    //assert(floor(buf_size / 26) != ceil(buf_size / 26));
    uint64_t m = buf_size / 26;
    uint64_t expected_transmission_size = 31 * m;

    int total_bytes_added_not_sent = expected_transmission_size;
    uint64_t bytes_not_sent = expected_transmission_size;

    /////////////////
    // send header
    /////////////////
    buf_raw[0] = ((uint64_t)(0xFF)) & ((uint64_t)total_zeros_added);
    buf_raw[1] = ((uint64_t)(0xFF)) & (expected_transmission_size >> (8 * 7));
    buf_raw[2] = ((uint64_t)(0xFF)) & (expected_transmission_size >> (8 * 6));
    buf_raw[3] = ((uint64_t)(0xFF)) & (expected_transmission_size >> (8 * 5));
    buf_raw[4] = ((uint64_t)(0xFF)) & (expected_transmission_size >> (8 * 4));
    buf_raw[5] = ((uint64_t)(0xFF)) & (expected_transmission_size >> (8 * 3));
    buf_raw[6] = ((uint64_t)(0xFF)) & (expected_transmission_size >> (8 * 2));
    buf_raw[7] = ((uint64_t)(0xFF)) & (expected_transmission_size >> (8 * 1));
    buf_raw[8] = ((uint64_t)(0xFF)) & (expected_transmission_size >> (8 * 0));
    for (int i = 9; i < 26; i++) buf_raw[i] = 0;
    encode_26_block_to_31(buf_enc, buf_raw);
    safe_send(sock, buf_enc, 31);
    bytes_not_sent -= 31;
    m--;

    /////////////////////////////////////////
    // send everything except the last block
    //////////\///////////////////////////////
    printd("raw: \n");
    while (bytes_left_to_read > 26) {
        fread(buf_raw, sizeof(uint8_t), 26, fp);
        bytes_left_to_read -= 26;
        for (int i=0; i<26; i++) printd("%02x ", buf_raw[i]);
        printd("\n");
        encode_26_block_to_31(buf_enc, buf_raw);
        safe_send(sock, buf_enc, 31);
        (*file_total_sent) += 31;
        bytes_not_sent -= 31;
        m--;
    }

    ///////////////////////////////////////////////////
    // send the last block padded by zeros from the end
    ///////////////////////////////////////////////////
    printd("Entering the danger zone! 26-bytes_missing=%d\n", (uint8_t)26 - bytes_missing_for_26);
    if (bytes_left_to_read > 0) {
        //fread(buf_raw, sizeof(uint8_t), bytes_left_to_read, fp);
        for (int i = 0; i < bytes_left_to_read; i++) {
            fread(buf_hold, sizeof(uint8_t), 1, fp);
            buf_raw[i] = buf_hold[0];
        }
        for (int i = (26-bytes_missing_for_26); i < 26; i++) buf_raw[i] = 0;
        for (int i = 0; i < 26; i++) printd("%02x ", buf_raw[i]);
        printd("\n");
        encode_26_block_to_31(buf_enc, buf_raw);
        safe_send(sock, buf_enc, 31);
        (*file_total_sent) += 31;
        bytes_not_sent -= 31;
        m--;
    }
    printd("\n");
    printd("closed\n");

    fclose(fp);
    return 0;
}

int main(const int argc, const char *argv[])
{
    char*   remote_addr;
    u_short remote_port;
    SOCKET sock;
    WSADATA wsaData;
    int status;
    char buf_tmp[1];

    char file_name[MAX_PERMITTED_FILE_PATH_LENGTH];
    uint64_t file_size, file_total_sent;

    if (argc != 3) {
        remote_addr = CHANNEL_ADDR;
        remote_port = CHANNEL_PORT_SENDER;
        printd("WARNING: proper syntax is as follows:\n");
        printd("         %s IP PORT\n", argv[0]);
        printd("         an invalid number of arguments was specified, so using IP=%s, PORT=%d\n", remote_addr, remote_port);
    } else {
        remote_addr = argv[1];
        remote_port = (u_short) atoi(argv[2]);
    }

    if (socket_initialize(&wsaData) != NO_ERROR) {
        printf(MSG_ERR_WSASTARTUP);
        return 1;
    }

    printd("Attempting connection to %s:%d\n", remote_addr, remote_port);
    while (true) {
        status = socket_connect(&sock, remote_addr, remote_port);
#if FLAG_IGNORE_SOCKET == 1
        break;
#endif
        if (status == STATUS_SUCCESS) {
            break;
        }
        /*else {
            printf(MSG_ERR_CONNECTING, status, remote_addr, remote_port);
        }*/
    }

    printd("Connected to %s:%d\n", remote_addr, remote_port);


    while (1) {
        printf(MSG_ENTER_FILENAME);
#if FLAG_SKIP_FILENAME==1
        strncpy_s(file_name, 100, DEBUG_FILE_PATH, strlen(DEBUG_FILE_PATH));
#else
        scanf_s("%s", file_name, MAX_PERMITTED_FILE_PATH_LENGTH);
        if (strcmp(file_name, "quit") == 0) {
            return 0;
        }
#endif

        // wait for channel to send "ok to send"
        printd("Waiting for server to give the OK...\n");
        int is_ok_to_send = 0;
        while (!is_ok_to_send) {
            safe_recv(sock, buf_tmp, 1);
            if (buf_tmp[0] == 1) {
                // ok
                is_ok_to_send = 1;
            }
            else {
                is_ok_to_send = 0;
            }
        }
        printd("Got OK\n");
        

        printd("Sending file...\n");
        status = socket_send_file(sock, file_name, &file_size, &file_total_sent);
        switch (status) {
            case STATUS_SUCCESS: {
                printf(MSG_FILE_LENGTH, file_size);
                printf(MSG_TOTAL_SENT,  file_total_sent);
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
                printf(MSG_ERR_MALLOC_BUF_ENC, file_total_sent);
                break;
            }
            default: {
                printf(MSG_ERR_UNKNOWN, status);
                break;
            }
        }
        closesocket(sock);
#if FLAG_SINGLE_ITER==1
        break;
#endif
    }

    return 0;
}