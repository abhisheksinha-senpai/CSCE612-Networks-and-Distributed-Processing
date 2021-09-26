#pragma once
#include "Socket.h"
#include "pch.h"


#define MAX_HOST_LEN		256
#define MAX_URL_LEN			2048
#define MAX_REQUEST_LEN		2048

class Crawler {
private:
	
public:
	LONG volatile nLinks = 0;
	LONG volatile Hostunique = 0;
	LONG volatile DNSLookups = 0;
	LONG volatile IPUnique = 0;
	LONG volatile robot = 0;
	LONG volatile http_check = 0;
	LONG volatile total_link = 0;
	HANDLE	finished;
	HANDLE	eventQuit;
	void stats(long start_time);
	void Initialize(char* str, int numThreads);
	bool Producer(char* fileName);
};

void Consumer(volatile LONG* nLinks, volatile LONG* Hostunique, volatile LONG* IPUnique);

