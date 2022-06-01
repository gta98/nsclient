// FileTransferChannel.c : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>

#include <string.h>
#include <sys/types.h>
#define SA struct sockaddr
#include "FileTransferCommon/common.h"
#pragma warning(disable:4996)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

enum channel_mode_t { DETERMINISTIC = 0, RANDOM = 1, NONE = 2 };

void check_args(int argc, char* argv[]);
int is_number(char* string);
int fake_noise_random(char* buffer, double p, unsigned int seed);
void open_socket(SOCKET* new_sock, struct sockaddr_in* sock_add, int port);

int main2(int argc, char* argv[]) {
	char* remote_addr;
	u_short remote_port;
	SOCKET sock;
	WSADATA wsaData;
	int status;
	check_args(argc, argv);
	if (socket_initialize(&wsaData) != NO_ERROR) {
		printf(MSG_ERR_WSASTARTUP);
		return 1;
	}


}

int sender_OK = 0;

void sigpipe_handler()
{
	printf("SIGPIPE caught\n");
	sender_OK = 0;
}

int main(int argc, char* argv[])
{
	int status;
	SOCKET* sockfd_sender;
	SOCKET* sockfd_recv;
	int accept_res_sender, accept_res_recv, len_send, len_recv;
	struct sockaddr_in sender_addr, receiver_addr, channel_addr;
	WSADATA wsaData;
	enum channel_mode_t channel_mode;

main_start:
	printd("FileTransferChannel initiated\n");

	if (FLAG_DEBUG == 0) {
		check_args(argc, argv);
	}

	if (argc == 1) {
		channel_mode = NONE;
	}
	else if (strcmp(argv[1], "-r") == 0) {
		channel_mode = RANDOM;
	}
	else if (strcmp(argv[1], "-d") == 0) {
		channel_mode = DETERMINISTIC;
	}




	if (socket_initialize(&wsaData) != NO_ERROR) {
		printf(MSG_ERR_WSASTARTUP);
		return 1;
	}

	status = socket_listen(&sockfd_sender, &channel_addr, CHANNEL_PORT_SENDER);
	if (status != STATUS_SUCCESS) {
		printf(MSG_ERR_SOCK_LISTEN, CHANNEL_PORT_SENDER);
		return status;
	}

	status = socket_listen(&sockfd_recv, &channel_addr, CHANNEL_PORT_RECEIVER);
	if (status != STATUS_SUCCESS) {
		printf(MSG_ERR_SOCK_LISTEN, CHANNEL_PORT_RECEIVER);
		return status;
	}

	// socket create and verification

	len_send = sizeof(&sender_addr);
	len_recv = sizeof(&receiver_addr);

	// Accept the data packet from client and verification
	
	// both sender and receiver must be connected at the same time in order for this to work!
	printd("Waiting for sender - accept()...\n");
	accept_res_sender = accept(sockfd_sender, NULL, NULL);
	printd("Waiting for receiver - accept()...\n");
	accept_res_recv = accept(sockfd_recv, NULL, NULL);

	if (accept_res_sender < 0) {
		printf("Sender is not connected! Aborting.\n");
	}

	if (accept_res_recv   < 0) {
		printf("Receiver is not connected! Aborting.\n");
	}

	if ((accept_res_sender < 0) || (accept_res_recv < 0)) {
		printf("See error (WSA error): %ld\n", WSAGetLastError());
		return 1; // FIXME - if this happens, we might wanna just go back to the beginning of the loop
	}
	
	printd("Sender and receiver connected!\n");
	// at this point, we assume both are connected

	char buf_tell_sender_to_start[1];
	buf_tell_sender_to_start[0] = 1;
	safe_send(sockfd_sender, buf_tell_sender_to_start, 1);

	int addlen = sizeof(sockfd_sender);

	//==============intialize buffers for messagge and ack============
	char buffer[31]; 
	//char ack[100];

	//============initialize prameters for select function==============
	int sock_avl;
	uint64_t countTot = 0, cur_count = 0, flipped_bits = 0;
	struct timeval tm;
	tm.tv_sec = 0;
	tm.tv_usec = 50000;
	fd_set readfds, writefds;
	/////////---------------------Tx-RX flow ----------------------------------------------------------------------------------
	while (1) {
		// 
		/*FD_ZERO(&readfds);
		FD_SET(sockfd_sender, &readfds);
		FD_SET(sockfd_recv, &readfds);
		sock_avl = select(max(sockfd_recv, sockfd_sender) +1, &readfds, NULL, NULL, &tm);

		if (sock_avl < 0) {
			printf("Could not select\n");
			return EXIT_FAILURE;
		}*/

		if (1) {//FD_ISSET(sockfd_sender, &readfds)) { //receiving data from client
			cur_count = 1;
			while (cur_count != 0) {
				printd("Waiting to receive from sender\n");
				cur_count = recvfrom(sockfd_sender, buffer, 1, 0, (struct sockaddr*)&sockfd_sender, &addlen);
				safe_recv(&sockfd_sender, buffer, 1);
				printd("cur_count = %d\n", cur_count);
				countTot += cur_count;
				if (channel_mode == RANDOM) {
					double probabilty = atoi(argv[2]) / pow(2, 16);
					if (probabilty > 1) probabilty = 1;
					flipped_bits += fake_noise_random(buffer, probabilty, atoi(argv[3]));
					sendto(sockfd_recv, buffer, cur_count, 0, (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
				}
				else if (channel_mode == DETERMINISTIC)
				{
					flipped_bits += fake_noise_determ(buffer, argv[2]);
					sendto(sockfd_recv, buffer, cur_count, 0, (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
				}
				else if (channel_mode == NONE) {
					// no noise
					flipped_bits += 0;
					sendto(sockfd_recv, buffer, cur_count, 0, (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
				}
			}

			closesocket(sockfd_sender);
			closesocket(sockfd_recv);
			
			/*char* next;
			bit keep_question_loop = 1;
			bit should_break;
			while (keep_question_loop) {
				printf("Continue? (Y/N): \n");
				scanf_s("%s", &next);
				if ((strcmp(next, "Y") == 0) || (strcmp(next, "y") == 0)) {
					should_break = 1;
					keep_question_loop = 0;
				} else if ((strcmp(next, "N") == 0) || (strcmp(next, "n") == 0)) {
					should_break = 0;
					keep_question_loop = 0;
				}
				else {
					keep_question_loop = 1;
					printf("Invalid response!\n");
				}
			}
			if (should_break == 1) break;*/

		}

		break;


	}
	//==================================closing sockets==========================
	closesocket(sockfd_sender);
	closesocket(sockfd_recv);
	exit(EXIT_SUCCESS);

}

void check_args(int argc, char* argv[])
{
	if (!((argc==3) || (argc==4))) {
		perror("wrong number of arguments, channel requiers 2 or 3 arguments.");
		exit(EXIT_FAILURE);
	}
	if ((strcmp(argv[1], "-r") != 0) && (strcmp(argv[1], "-d") != 0)) {
		perror("wrong argument, must choose the noise method");
		exit(EXIT_FAILURE);
	}
	if (argc == 4) {
		if (!is_number(argv[3])) {
			perror("random seed has to be a number");
			exit(EXIT_FAILURE);
		}
		if (!is_number(argv[2])) {
			perror("probabilty has to be a number");
		}
		if (atoi(argv[2]) > pow(2, 16) || atoi(argv[3]) < 0)
		{
			perror("p/2^16 is in range 0-1");
			exit(EXIT_FAILURE);
		}
	}
	else
		if (!is_number(argv[2]) && atoi(argv[2]) > 0)
		{
			perror("length of cycle nust be positive integer");
			exit(EXIT_FAILURE);
		}
}

//checking is string is a number
int is_number(char* string)
{
	for (int i = 0; i < strlen(string); i++)
	{
		if (string[i] < '0' || string[i]>'9')
			return 0;
	}
	return 1;
}

//flip bits randomly with a given probabilty by the given seed
int fake_noise_random(char* buffer, double p, unsigned int seed) {
	srand(seed);
	int mask, max_rand = 1 / p, count = 0;
	int flag = (p == 1 / pow(2, 16));
	for (int i = 0; i < 2040; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			mask = pow(2, j);
			if (rand() % max_rand == 1)// a random lottery with the defined probability for every single bit
				if (flag == 0) {
					buffer[i] = (char)(buffer[i] ^ mask);
					count++;
				}
				else if (flag == 1 && rand() % 2 == 0) {
					buffer[i] = (char)(buffer[i] ^ mask);
					count++;
				}

		}
	}
	return count;
}

int fake_noise_determ(char* buffer, int n)
{
	int mask = 1;
	int count = 0;
	for (int i = 0; i < 2040; i++)
	{
		if (i % n == 0) {
			buffer[i] = (char)(buffer[i] ^ mask);
			count++;
		}
	}
	return count;
}

void open_socket(SOCKET* new_sock, struct sockaddr_in* sock_add, int port)
{
	*new_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (*new_sock == -1) {
		printf(MSG_ERR_CREATE_SOCK);
		exit(EXIT_FAILURE);
	}
	else printd("Socket successfully created..\n");
	sock_add->sin_family = AF_INET;
	sock_add->sin_addr.s_addr = htonl(INADDR_ANY);
	sock_add->sin_port = htons(port);
	if ((bind(*new_sock, (SA*)&sock_add, sizeof(sock_add))) != 0) {
		printf(MSG_ERR_SOCK_BIND, port);
		exit(0);
	}
	else
		printd("Socket successfully binded..\n");
	if ((listen(*new_sock, 5)) != 0) {
		printd("Listen failed...\n");
		exit(0);
	}
	else
		printd("Server listening..\n");
	return;
}
