#pragma once
#include "Utilities.h"
#include "ICMPHeader.h"
#include "IPHeader.h"
class ParallelTrace
{
	int64_t start_time;
	WSADATA wsaData;
	double* EstimatedRTT;
	//double *devRTT;
	int* retx_count;
	std::string* source_ip;
	int64_t* sent_time;
	double* Estimated_RTO;
	char* ICMP_reply_buf;
	char* ICMP_send_buf;
	int* rcv_type;
	int* rcv_code;

	int finish_index = 0;
	SOCKET sock;
	sockaddr_in server;
	bool* received;
	bool finished=false;
	std::string targetIP;

	HANDLE socketReceiveReady;
	HANDLE workerthread_DNS[40];
	bool workerQuits[40] = {false};

	int SendPacket(int hopNumber);
	int ReceivePacket();
	void MakePacket(int hopNumber);

	std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, compareFunc> future_retx;

public:
	std::queue<std::string> websites;
	ParallelTrace();
	void Traceroute(ULONG IP);
	void ICMP_Tx(int hopNumber);
	ULONG GetNextIP();
	void printTraceRoute();
	~ParallelTrace();
	void DynamicTimeout(int hopNumber);
};

