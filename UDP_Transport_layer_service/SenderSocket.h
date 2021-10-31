#pragma once
#include "pch.h"
#include "UDP_Transport.h"

class SenderSocket
{private:
	SOCKET sock;
	SenderSynHeader sd_head;
	sockaddr_in local;
	sockaddr_in server;
	hostent* remote;
	clock_t start;
	float transfer_start;
	bool socketOpened;
	bool socketClosed;
	int myport;
	char* mytarget;
	double Estimated_RTO = 1.0;
	int Transport_Sender(char* target, int magic_port);
public:
	SenderSocket();
	int Open(char* target, int magic_port, int sender_window_size, LinkProperties& lp);
	int Close();
	int Send(char* buf, int size);
	~SenderSocket();
};

