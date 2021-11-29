#pragma once
#include "pch.h"
#include "UDP_Transport.h"
#include "CheckSum.h"
#include "Packet.h"

#pragma pack(push,1)
class SenderSocket
{public:
	SOCKET sock;
	SenderSynHeader sd_head;
	SenderDataHeader sd_send;
	sockaddr_in local;
	sockaddr_in server;
	hostent* remote;
	CheckSum cs;
	ReceiverHeader recevied_header;
	clock_t endtime;
	sockaddr_in response_addr;
	int sz = sizeof(response_addr);
	double sampleRTT;
	int newReleased;
	int status;
	int response;

	float devRTT = 0.0;
	float transfer_start;
	bool socketOpened;
	bool socketClosed;
	int myport;
	char* mytarget;
	double Estimated_RTO = 1.0;
	unsigned char UDP_buf[1500];
	int retransmit_count = 0;

	int Transport_Sender(char* target, int magic_port);
public:
	DWORD ackSeqbyreceiver;
	DWORD sendBasenumber;
	DWORD nextToSend;
	DWORD nextSeqnumber;
	DWORD checksum;
	
	clock_t start;
	clock_t sendstart;
	clock_t sendend;
	double timerexpire;

	HANDLE eventQuit;
	HANDLE workerQuit;
	HANDLE errorQuit;
	HANDLE printQuit;
	HANDLE sendFinish;

	HANDLE queueEmpty;
	HANDLE socketReceiveReady;
	HANDLE queueFull;

	HANDLE handles_thread_worker;
	HANDLE handles_thread_stat;

	int fast_tx_count = 0;
	int windowSize;
	int rcvWindsize;
	float EstimatedRTT = 0.0;
	int lastReleased;

	long dwordbufferSize;

	Packet *pending_pkts;
	bool* retx_pkts;
	bool* istransmitted;
	int* retx_cnts;
	char* userBuf;
public:
	SenderSocket();
	int Open(char* target, int magic_port, int sender_window_size, LinkProperties& lp);
	int Close();
	int Send(char* buf, int size);
	int ReceiveACK(void);
	~SenderSocket();
};
#pragma pack(pop)

