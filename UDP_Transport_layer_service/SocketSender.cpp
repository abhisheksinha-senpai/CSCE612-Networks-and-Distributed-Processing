#include "pch.h"
#include "SenderSocket.h"

int SenderSocket::Open(char* target, int magic_port, int sender_window_size, LinkProperties& lp)
{
	if (socketOpened)
		return ALREADY_CONNECTED;
	socketOpened = true;
	sd_head.sdh.flags.SYN = 1;
	sd_head.sdh.seq = 0;
	memcpy(&sd_head.lp, &lp, sizeof(LinkProperties));
	mytarget = (char*)malloc(strlen(target) + 1);
	memcpy(mytarget, target, strlen(target) + 1);
	myport = magic_port;
	Estimated_RTO = max(1, 3*lp.RTT);
	int count = 0;
	clock_t end, before_send, temp = clock();
	while (count++ < SYN_ATTEMPTS)
	{
		before_send = clock();
		int status = Transport_Sender(target, magic_port);
		if (status != STATUS_OK)
			return status;
		end = clock();
		printf("[ %.3f]  --> SYN %d (attempt %d of %d, RTO %.3f) to %s\n", ((float)(end - start) / CLOCKS_PER_SEC), sd_head.sdh.seq, count, SYN_ATTEMPTS, Estimated_RTO, inet_ntoa(server.sin_addr));

		timeval tp = { (long)Estimated_RTO,(Estimated_RTO - (long)Estimated_RTO)*1000000 };
		fd_set fd;
		FD_ZERO(&fd); // clear the set
		FD_SET(sock, &fd); // add your socket to the set
		int available = select(0, &fd, NULL, NULL, &tp);
		ReceiverHeader recevied_header;
		sockaddr_in response_addr;
		int sz = sizeof(response_addr);
		if (available > 0)
		{
			int response = recvfrom(sock, (char*)&recevied_header, sizeof(ReceiverHeader), 0, reinterpret_cast<struct sockaddr*>(&response_addr), &sz);
			
			if (response == SOCKET_ERROR)
			{
				end = clock();
				printf("[ %.3f]  --> failed recvfrom with %d\n", ((float)(end - start) / CLOCKS_PER_SEC), WSAGetLastError());
				return FAILED_RECV;
			}
			end = clock();
			transfer_start = ((float)((float)end - (float)start) / CLOCKS_PER_SEC);
			Estimated_RTO = (3*(double)((float)end - (float)before_send)) / CLOCKS_PER_SEC;
			printf("[ %.3f]  <-- SYN-ACK %d window %d; setting initial RTO to %.3f\n", ((float)(end - start) / CLOCKS_PER_SEC), recevied_header.ackSeq, recevied_header.recvWnd, Estimated_RTO);
			printf("Main:  connected to %s in %.3f sec, pkt size %d bytes\n", target, ((float)((float)end - (float)temp) / CLOCKS_PER_SEC), response);
			return STATUS_OK;
		}
	}
	
	return TIMEOUT;
}

SenderSocket::SenderSocket()
{
	start = clock();
    WSADATA wsaData;
    //Initialize WinSock; once per program run
    WORD wVersionRequested = MAKEWORD(2, 2);
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        printf("WSAStartup error %d\n", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(0);
	if (bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
	{
		printf("socket() binding error %d\n", WSAGetLastError());
		closesocket(sock);
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	socketOpened = false;
	socketClosed = false;

}

int SenderSocket::Transport_Sender(char *target, int magic_port)
{
	
	DWORD IP_add = inet_addr(target);
	server.sin_family = AF_INET;
	server.sin_port = htons(magic_port);
	if (IP_add == INADDR_NONE)
	{
		if ((remote = gethostbyname(target)) == NULL)
		{
			clock_t end = clock();
			printf("[ %.3f]  --> target %s is invalid\n", ((float)(end - start) / CLOCKS_PER_SEC), target);
			return INVALID_NAME;
		}
		else
			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
	}
	else
		server.sin_addr.S_un.S_addr = IP_add;
	int status = sendto(sock, (char*)&sd_head, sizeof(SenderSynHeader), 0, (struct sockaddr*)&server, sizeof(server));
	if (status == SOCKET_ERROR)
	{
		clock_t end = clock();
		printf("[ %.3f]  --> failed sendto with %d\n", ((float)(end - start) / CLOCKS_PER_SEC), WSAGetLastError());
		return FAILED_SEND;
	}
	return STATUS_OK;
}

int SenderSocket::Close()
{
	if (!socketOpened)
		return NOT_CONNECTED;
	sd_head.sdh.flags.FIN = 1;
	sd_head.sdh.flags.SYN = 0;
	sd_head.sdh.flags.ACK = 0;
	sd_head.sdh.seq = 0;
	int count = 0;
	clock_t end;
	bool flagOnce = 0;
	float time_next;
	while (count++ < FIN_ATTEMPTS)
	{
		int status = Transport_Sender(mytarget, myport);
		if (status != STATUS_OK)
			return status;
		end = clock();
		if (!flagOnce)
		{
			time_next = ((float)((float)clock() - (float)start) / CLOCKS_PER_SEC);
			flagOnce = true;
		}
		printf("[ %.3f]  --> FIN %d (attempt %d of %d, RTO %.3f)\n", ((float)(end - start) / CLOCKS_PER_SEC), sd_head.sdh.seq, count, SYN_ATTEMPTS, Estimated_RTO);
		timeval tp = { (long)Estimated_RTO,(Estimated_RTO - (long)Estimated_RTO) * 1000000 };
		fd_set fd;
		FD_ZERO(&fd); // clear the set
		FD_SET(sock, &fd); // add your socket to the set
		int available = select(0, &fd, NULL, NULL, &tp);
		ReceiverHeader recevied_header;
		sockaddr_in response_addr;
		int sz = sizeof(response_addr);
		if (available > 0)
		{
			int response = recvfrom(sock, (char*)&recevied_header, sizeof(ReceiverHeader), 0, reinterpret_cast<struct sockaddr*>(&response_addr), &sz);
			
			if (response == SOCKET_ERROR)
			{
				end = clock();
				printf("[ %.3f]  --> failed recvfrom with %d\n", ((float)(end - start) / CLOCKS_PER_SEC), WSAGetLastError());
				return FAILED_RECV;
			}
			end = clock();
			printf("[ %.3f]  <-- FIN-ACK %d window %d\n", ((float)(end - start) / CLOCKS_PER_SEC), recevied_header.ackSeq, recevied_header.recvWnd);
			printf("Main:  transfer finished in %.3f sec\n", (float)time_next -  (float)transfer_start);
			return STATUS_OK;
		}
	}
	return TIMEOUT;
}

int SenderSocket::Send(char *buf, int size)
{
	return 0;
}

SenderSocket::~SenderSocket()
{
	free(mytarget);
	closesocket(sock);
	WSACleanup();
}