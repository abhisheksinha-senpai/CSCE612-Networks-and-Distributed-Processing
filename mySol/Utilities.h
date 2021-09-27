#pragma once
#include "Socket.h"
#include "pch.h"


#define MAX_HOST_LEN		256
#define MAX_URL_LEN			4096
#define MAX_REQUEST_LEN		256


class Crawler {
private:
	
public:
	LONG volatile nLinks = 0;
	LONG volatile Hostunique = 0;
	LONG volatile DNSLookups = 0;
	LONG volatile IPUnique = 0;
	LONG volatile robot = 0;
	LONG volatile http_check2 = 0;
	LONG volatile http_check3 = 0;
	LONG volatile http_check4 = 0;
	LONG volatile http_check5 = 0;
	LONG volatile other = 0;
	LONG volatile total_link = 0;
	LONG volatile dataBytes = 0;
	LONG volatile QueueUsed = 0;
	LONG volatile numberThreads = 0;
	HANDLE hEvent;
	void Initialize(char* str);
	bool Producer(char* fileName);
};

void Consumer(LPVOID pParam);
void stats(LPVOID pParam);
void final_stat(LPVOID pParam);

