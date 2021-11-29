#include "pch.h"
#include "SenderSocket.h"

unsigned long long int last_size = 0;

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
	Estimated_RTO = max(1, 2 * lp.RTT);
	EstimatedRTT = lp.RTT;
	devRTT = 0.0;
	int count = 0;
	windowSize = sender_window_size;
	pending_pkts = new Packet[windowSize];
	retx_pkts = new bool[windowSize];
	retx_cnts = new int[windowSize];
	istransmitted = new bool[windowSize];
	memset(retx_pkts, 0, windowSize * sizeof(bool));
	memset(pending_pkts, 0, windowSize*sizeof(Packet));
	memset(retx_cnts, 0, windowSize * sizeof(int));
	memset(istransmitted, 0, windowSize * sizeof(bool));
	clock_t end, before_send, temp = clock();
	while (count++ < SYN_ATTEMPTS)
	{
		before_send = clock();
		int status = Transport_Sender(target, magic_port);
		if (status != STATUS_OK)
			return status;
		end = clock();
		//printf("[ %.3f]  --> SYN %d (attempt %d of %d, RTO %.3f) to %s\n", ((float)(end - start) / CLOCKS_PER_SEC), sd_head.sdh.seq, count, SYN_ATTEMPTS, Estimated_RTO, inet_ntoa(server.sin_addr));

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
			if (recevied_header.flags.SYN == 1 && recevied_header.flags.ACK==1)
			{
				float sampleRTT = ((float)end - (float)(before_send)) / CLOCKS_PER_SEC;
				EstimatedRTT = (1 - 0.125) * EstimatedRTT + 0.125 * sampleRTT;
				devRTT = (1 - 0.25) * devRTT + 0.25 * abs(sampleRTT - EstimatedRTT);
				Estimated_RTO = EstimatedRTT + 4 * max(devRTT, 0.010);
				//printf("[ %.3f]  <-- SYN-ACK %d window %d; setting initial RTO to %.3f\n", ((float)(end - start) / CLOCKS_PER_SEC), recevied_header.ackSeq, recevied_header.recvWnd, Estimated_RTO);
			}
			else
				continue;
			sendBasenumber = recevied_header.ackSeq;
			nextSeqnumber = sendBasenumber;
			queueFull = CreateSemaphore(NULL, 0, windowSize, NULL);
			lastReleased = min(windowSize, recevied_header.recvWnd);
			queueEmpty = CreateSemaphore(NULL, 0, windowSize, NULL);
			ReleaseSemaphore(queueEmpty, lastReleased, NULL);
			handles_thread_worker = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkerRun, this, 0, NULL);
			handles_thread_stat = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Stats, this, 0, NULL);
			rcvWindsize = recevied_header.recvWnd;
			sd_send.flags.ACK = 0;
			sd_send.flags.SYN = 0;
			sd_send.flags.FIN = 0;
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
	
	printQuit = CreateEvent(NULL, true, false, NULL);
	eventQuit = CreateEvent(NULL, true, false, NULL);
	workerQuit = CreateEvent(NULL, true, false, NULL);
	errorQuit = CreateEvent(NULL, true, false, NULL);
	sendFinish = CreateEvent(NULL, true, false, NULL);
	socketReceiveReady = CreateEvent(NULL, false, false, NULL);
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
	SetEvent(sendFinish);
	WaitForSingleObject(workerQuit, INFINITE);
	CloseHandle(handles_thread_worker);
	sd_head.sdh.flags.FIN = 1;
	sd_head.sdh.flags.SYN = 0;
	sd_head.sdh.flags.ACK = 0;
	sd_head.sdh.seq = sendBasenumber;
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
		//printf("[ %.3f]  --> FIN %d (attempt %d of %d, RTO %.3f)\n", ((float)(end - start) / CLOCKS_PER_SEC), sd_head.sdh.seq, count, FIN_ATTEMPTS, Estimated_RTO);
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
			DWORD check = cs.CRC32((unsigned char*)userBuf, last_size);
			if (recevied_header.flags.ACK == 1 && recevied_header.flags.FIN == 1)
			{
				end = clock();
				sendBasenumber = recevied_header.ackSeq;
				printf("[ %.3f]  <-- FIN-ACK %d window %X\n", ((float)(end - start) / CLOCKS_PER_SEC), recevied_header.ackSeq, recevied_header.recvWnd);
				checksum = check;
				SetEvent(printQuit);
				int st = WaitForSingleObject(handles_thread_stat, INFINITE);
				CloseHandle(handles_thread_stat);
				printf("Main:  transfer finished in %.3f sec, %.3f Kbps, checksum %X\n", ((float)((float)end - (float)start) / CLOCKS_PER_SEC), (8 * dwordbufferSize) / (1024 * (float)((float)sendend - (float)sendstart) / CLOCKS_PER_SEC), checksum);
				return STATUS_OK;
			}
		}
	}
	
	return TIMEOUT;
}

int SenderSocket::Send(char* data, int size)
{
	HANDLE events[] = { queueEmpty, errorQuit};
	int ret = WaitForMultipleObjects(2, events, false, INFINITE);
	switch (ret)
	{
	case WAIT_OBJECT_0:
	{
		DWORD slot = nextSeqnumber % windowSize;
		Packet* p = pending_pkts + slot; // pointer to packet struct
		SenderDataHeader* sdh =(SenderDataHeader*)(&(p->pkt[0]));
		sdh->seq = nextSeqnumber;
		memcpy(sdh + 1, data, size);
		sdh->flags.magic = MAGIC_PROTOCOL;
		/*sd_send.seq = nextSeqnumber;
		memcpy((unsigned char*)&(p->pkt) + sizeof(SenderDataHeader), data, size);
		memcpy((unsigned char*)&(p->pkt), (unsigned char*)&sd_send, sizeof(SenderDataHeader));*/
		p->txTime = 0;
		p->size = size;
		nextSeqnumber++;
		ReleaseSemaphore(queueFull, 1, NULL);
		break;
	}
	case WAIT_OBJECT_0 +1:
		return FAILED_SEND;
	default:
	{
		printf("[ %.3f]  Wating for queueEmpty failed with error code:%d\n", ((float)(clock() - start) / CLOCKS_PER_SEC),GetLastError());
		return RETRANSMIT;
		break;
	}
	}
	return STATUS_OK;
}



SenderSocket::~SenderSocket()
{
	free(mytarget);
	free(pending_pkts);
	free(retx_pkts);
	free(retx_cnts);
	free(istransmitted);
	closesocket(sock);
	WSACleanup();
}

UINT WorkerRun(LPVOID pParam)
{
	clock_t endtime;
	SenderSocket* ss = (SenderSocket*)pParam;
	ss->nextToSend = ss->sendBasenumber;
	int kernelBuffer = 20e6; // 20 meg
	if (setsockopt(ss->sock, SOL_SOCKET, SO_RCVBUF, (char *)&kernelBuffer, sizeof(int)) == SOCKET_ERROR)
	{
		endtime = clock();
		printf("[ %.3f]  Unable to set the receiver kernel buffer size to 20 MB with error code: %d\n", ((float)(endtime - ss->start) / CLOCKS_PER_SEC), WSAGetLastError());
	}
	kernelBuffer = 20e6;
	if (setsockopt(ss->sock, SOL_SOCKET, SO_SNDBUF, (char*)&kernelBuffer, sizeof(int)) == SOCKET_ERROR)
	{
		endtime = clock();
		printf("[ %.3f]  Unable to set the Sender kernel buffer size to 20 MB with error code: %d\n", ((float)(endtime - ss->start) / CLOCKS_PER_SEC), WSAGetLastError());
	}
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	if (WSAEventSelect(ss->sock, ss->socketReceiveReady, FD_READ) == SOCKET_ERROR)
	{
		endtime = clock();
		printf("[ %.3f]  Error in socket assoication of object with error code:%d\n", ((float)(endtime - ss->start) / CLOCKS_PER_SEC), WSAGetLastError());
		SetEvent(ss->errorQuit);
		return FAILED_SEND;
	}
	HANDLE events[] = { ss->socketReceiveReady, ss->queueFull, ss->workerQuit };
	clock_t timeout;
	double cur_time = ((double)clock() - ss->start) / CLOCKS_PER_SEC;
	ss->timerexpire = ss->Estimated_RTO + cur_time;
	while (true)
	{
		cur_time = ((double)clock() - ss->start) / CLOCKS_PER_SEC;
		if (ss->sendBasenumber != ss->nextSeqnumber)
		{
			timeout = 1000 * (ss->timerexpire - cur_time);
		}
		else
		{
			if (WaitForSingleObject(ss->sendFinish, 0) == WAIT_OBJECT_0)
				SetEvent(ss->workerQuit);
			timeout = INFINITE;
		}
		int ret = WaitForMultipleObjects(3, events, false, timeout);
		switch (ret)
		{
		case WAIT_TIMEOUT:	
		{
			if (ss->retx_cnts[ss->sendBasenumber % ss->windowSize] == RETRANSMIT_ATTEMPTS_PKT)
			{
				printf("[ %.3f]  Maximum retransmissions reached\n");
				SetEvent(ss->errorQuit);
				return TIMEOUT;
			}

			ss->istransmitted[ss->sendBasenumber % ss->windowSize] = false;
			ss->timerexpire = cur_time + ss->Estimated_RTO;
			ss->pending_pkts[ss->sendBasenumber % ss->windowSize].txTime = cur_time;
			ss->status = sendto(ss->sock, ss->pending_pkts[ss->sendBasenumber % ss->windowSize].pkt, ss->pending_pkts[ss->sendBasenumber % ss->windowSize].size + sizeof(SenderDataHeader), 0, (struct sockaddr*)&(ss->server), sizeof(ss->server));

			if (ss->status == SOCKET_ERROR)
			{
				endtime = clock();
				printf("[ %.3f]  Sendto in timeout failed with error code: %d\n", ((float)(endtime - ss->start) / CLOCKS_PER_SEC), WSAGetLastError());
				SetEvent(ss->errorQuit);
				return FAILED_SEND;
			}
			ss->retransmit_count++;
			ss->retx_pkts [ss->sendBasenumber % ss->windowSize]= true;
			ss->timerexpire = cur_time + ss->Estimated_RTO;
			ss->retx_cnts[ss->sendBasenumber % ss->windowSize]++;
			break;
		}
		case WAIT_OBJECT_0:
		{
			// move senderBase; update RTT; handle fast retx; do flow control
			ss->status = ss->ReceiveACK();
			if (ss->status != STATUS_OK)
			{
				endtime = clock();
				SetEvent(ss->errorQuit);
				return FAILED_SEND;
			}
			break;
		}
		case WAIT_OBJECT_0 +1:
		{
			ss->pending_pkts[ss->nextToSend % ss->windowSize].txTime = cur_time;
			int status = sendto(ss->sock, ss->pending_pkts[ss->nextToSend % ss->windowSize].pkt, ss->pending_pkts[ss->nextToSend % ss->windowSize].size + sizeof(SenderDataHeader), 0, (struct sockaddr*)&(ss->server), sizeof(ss->server));
			if (ss->status == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
			{
				endtime = clock();
				printf("[ %.3f]  Sendto failed with error code: %d\n", ((float)(endtime - ss->start) / CLOCKS_PER_SEC), WSAGetLastError());
				SetEvent(ss->errorQuit);
				return FAILED_SEND;
			}
			last_size += ss->pending_pkts[ss->nextToSend % ss->windowSize].size;
			if (ss->sendBasenumber == ss->nextToSend)
				ss->timerexpire = cur_time + ss->Estimated_RTO;
			ss->nextToSend++;
			break;
		}
		case WAIT_OBJECT_0 + 2:
		{
			if (ss->sendBasenumber == ss->nextSeqnumber)
			{
				SetEvent(ss->printQuit);
				ss->sendend = clock();
				return STATUS_OK;
			}
			break;
		}
		default:
		{
			endtime = clock();
			printf("[ %.3f]  Waiting for Multiple objects failed with error code: %d\n", ((float)(endtime - ss->start) / CLOCKS_PER_SEC), GetLastError());
			break;
		}
		}
	}
}

int SenderSocket::ReceiveACK(void)
{
	double cur_time = ((double)clock() - start) / CLOCKS_PER_SEC;
	response = recvfrom(sock, (char*)&recevied_header, sizeof(ReceiverHeader), 0, reinterpret_cast<struct sockaddr*>(&response_addr), &sz);
	if (response == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
	{
		endtime = clock();
		printf("[ %.3f]  --> failed recvfrom with %d\n", ((float)(endtime - start) / CLOCKS_PER_SEC), WSAGetLastError());
		return FAILED_RECV;
	}

	if (sendBasenumber == recevied_header.ackSeq)
	{
		retx_cnts[sendBasenumber % windowSize]++;
	}
	if(recevied_header.ackSeq > sendBasenumber)
	{
		if(!retx_pkts[(sendBasenumber) % windowSize])
		{
			sampleRTT = cur_time - (double)(pending_pkts[(recevied_header.ackSeq -1) % windowSize]).txTime;
			EstimatedRTT = (1 - 0.125) * EstimatedRTT + 0.125 * sampleRTT;
			devRTT = (1 - 0.25) * devRTT + 0.25 * abs(sampleRTT - EstimatedRTT);
			Estimated_RTO = EstimatedRTT + 4 * max(devRTT, 0.010);
		}
		
		timerexpire = cur_time + Estimated_RTO;
		sendBasenumber = recevied_header.ackSeq;
		newReleased = sendBasenumber + min(windowSize, recevied_header.recvWnd) - lastReleased;
		ReleaseSemaphore(queueEmpty, newReleased, NULL);
		lastReleased += newReleased;
	}
	if (retx_cnts[sendBasenumber % windowSize]==4  && !istransmitted[sendBasenumber % windowSize])
	{
		retx_pkts[sendBasenumber % windowSize] = true;
		istransmitted[sendBasenumber % windowSize] = true;
		fast_tx_count++;
		timerexpire = cur_time + Estimated_RTO;
		pending_pkts[sendBasenumber % windowSize].txTime = cur_time;
		status = sendto(sock, pending_pkts[sendBasenumber % windowSize].pkt, pending_pkts[sendBasenumber % windowSize].size + sizeof(SenderDataHeader), 0, (struct sockaddr*)&(server), sizeof(server));
		if (status == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
		{
			endtime = clock();
			printf("[ %.3f]  Fast Re-Transmit failed with error code: %d\n", ((float)(endtime - start) / CLOCKS_PER_SEC), WSAGetLastError());
			return FAILED_SEND;
		}
	}
	rcvWindsize = recevied_header.recvWnd;
	return STATUS_OK;
}