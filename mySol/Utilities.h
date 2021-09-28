#pragma once
#include "Socket.h"
#include "pch.h"


#define MAX_HOST_LEN		256
#define MAX_URL_LEN			4096
#define MAX_REQUEST_LEN		256


class Crawler {
private:

public:
	long long volatile nLinks = 0;
	long volatile Hostunique = 0;
	long volatile DNSLookups = 0;
	long volatile IPUnique = 0;
	long volatile robot = 0;
	long volatile http_check2 = 0;
	long volatile http_check3 = 0;
	long volatile http_check4 = 0;
	long volatile http_check5 = 0;
	long volatile other = 0;
	long long volatile total_link = 0;
	long long volatile dataBytes = 0;
	long volatile QueueUsed = 0;
	long volatile numberThreads = 0;
	long volatile pages = 0;
	HANDLE hEvent;
	bool Producer(char* fileName);
};

void Consumer(LPVOID pParam);
void stats(LPVOID pParam);

