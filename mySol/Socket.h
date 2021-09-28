/* Author: Abhishek Sinha
*  Class : CS612
* Semester: Fall 2021
*/

#pragma once
#include "pch.h"
#include <unordered_set>
#include <string>
#include "Utilities.h"

#define MAX_HOST_LEN		256
#define MAX_URL_LEN			4096
#define MAX_REQUEST_LEN		256


class Socket
{
private:
	bool queryPresent = false;
	bool portPresent = false;
	bool pathPresent = false;

public:
	unordered_set<DWORD> seenIPs;
	unordered_set<string> seenHosts;
	unordered_set<string> DNSsuccess;
	SOCKET sock;									//socket handle
	char* buff;										//current buffer
	int allocatedSize;								//bytes allocated for buffer
	int curPos;										//current position in the buffer
	int THRESHOLD;									//Threasholdbefore rellocating memory to the buffer


	char reqBuffer[MAX_URL_LEN];
	char* hostName = NULL;
	char* port_pos = NULL;
	char* pathpos = NULL;
	char* query = NULL;

	int DNSLooked = 0;
	int IPLooked = 0;
	int page_co = 0;
	int robot_looked = 0;
	int http_check2 = 0;
	int http_check3 = 0;
	int http_check4 = 0;
	int http_check5 = 0;
	int other = 0;

	Socket();										//default Constructor
	bool init_sock(const char* str, int x, LPVOID pParam);				//Intialize socket
	bool Read(int flag);								//Read from the sokcet
	bool Get(char* str, int flag, bool parse);					//GET Methods for URL
};