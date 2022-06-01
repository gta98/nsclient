// FileTransferSender.c : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "FileTransferCommon/common.h"

int socket_send_file(const SOCKET* sock, const char* file_name, uint64_t* file_size, uint64_t* file_total_sent);
int validateHost(const char* hostname);
struct hostent* dnsQuery(const char* hostname);

int main(const int argc, const char *argv[])
{
    char*   remote_addr;
    u_short remote_port;
    SOCKET sock;
    WSADATA wsaData;
    struct hostent* remoteHost;
    int status;
    DWORD dwError;
    int i;
    struct in_addr addr;

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
    else {
        remote_addr = argv[1];
        remote_port = DEFAULT_DNS_PORT; // (u_short)atoi(argv[2]);
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
        printf(MSG_ENTER_HOSTNAME);
#if FLAG_SKIP_FILENAME==1
        strncpy_s(file_name, 100, DEBUG_FILE_PATH, strlen(DEBUG_FILE_PATH));
#else
        scanf_s("%s", hostname, MAX_HOSTNAME_LENGTH_INPUT);
        if (strcmp(hostname, QUIT_COMMAND_STRING) == 0) {
            closesocket(sock);
            return 0;
        }
#endif

        status = validateHost(hostname);
        if (status != STATUS_SUCCESS) {
            printf(MSG_ERR_BAD_NAME);
            continue;
        }

        remoteHost = dnsQuery(hostname);

        if (remoteHost) {
            i = 0;
            if (remoteHost->h_addrtype == AF_INET)
            {
                while (remoteHost->h_addr_list[i] != 0) {
                    addr.s_addr = *(u_long*)remoteHost->h_addr_list[i++];
                    printf("%s\n", inet_ntoa(addr));
                }
            }
        }
        else {
            printf(MSG_ERR_NONEXISTENT);
        }

#if FLAG_SINGLE_ITER==1
        break;
#endif
    }

    return 0;
}

struct hostent* dnsQuery(const char* hostname) {
    struct hostent* remoteHost;
    remoteHost = gethostbyname(hostname);
    return remoteHost;
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


int socket_send_file(const SOCKET* sock, const char* file_name, uint64_t* file_size, uint64_t* file_total_sent) {
    unsigned char buf_send[4], buf_hold[1], buf_encode[4], buf_read[4], buf_send_enc[31];
    uint8_t buf_raw[26], buf_enc[31];
    uint8_t buf_enc_tmp[31];
    uint64_t file_size_left;
    errno_t err;
    FILE* fp;

    *file_size = 0;
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
        for (int i = 0; i < 26; i++) printd("%02x ", buf_raw[i]);
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
        for (int i = (26 - bytes_missing_for_26); i < 26; i++) buf_raw[i] = 0;
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