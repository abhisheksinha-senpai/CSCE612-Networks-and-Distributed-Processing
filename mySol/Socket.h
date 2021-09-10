/* Author: Abhishek Sinha
*  Class : CS612
* Semester: Fall 2021
*/

#pragma once
#include "pch.h"

#define MAX_HOST_LEN		256
#define MAX_URL_LEN			2048
#define MAX_REQUEST_LEN		2048

class Socket
{
public:
	SOCKET sock;									//socket handle
	char* buff;										//current buffer
	int allocatedSize;								//bytes allocated for buffer
	int curPos;										//current position in the buffer
	int THRESHOLD;									//Threasholdbefore rellocating memory to the buffer

	char reqBuffer[MAX_URL_LEN] = {'\0'};
	char hostName[MAX_HOST_LEN] = { '\0' };
	char path[MAX_HOST_LEN] = { '\0' };
	char queryBuffer[MAX_URL_LEN - MAX_HOST_LEN] = { '\0' };
	char fragBuffer[MAX_URL_LEN - MAX_HOST_LEN] = { '\0' };
	char port_num[MAX_HOST_LEN] = { '\0' };


	Socket();										//default Constructor
	bool init_sock(const char* str);				//Intialize socket
	bool Read(void);								//Read from the sokcet
	bool Get(char* str);							//GET Methods for URL
	void HTMLfileParser(char* str);
	void Display_Stats();
};

