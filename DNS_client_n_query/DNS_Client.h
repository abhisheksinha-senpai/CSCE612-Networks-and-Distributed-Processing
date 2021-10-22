#pragma once
#include "pch.h"
#include "DNS_query.h"

class DNS_Client
{
private:
	SOCKET sock;
	sockaddr_in local;
	sockaddr_in remote;
	char myhost[MAX_DNS_SIZE];
	int query_size = 0;
	char query[MAX_DNS_SIZE];
	char RR[MAX_DNS_SIZE];
	bool errorflag = 0;

public:
	DNS_Client();
	~DNS_Client();
	bool query_generator(int qt, char* host);
	int query_type_identifier(char* kworgs);
	void UDP_sender(char* dns);
	void response_parser(char* dns);
};

