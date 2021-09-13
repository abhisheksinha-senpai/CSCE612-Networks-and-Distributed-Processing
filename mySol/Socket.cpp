/* Author: Abhishek Sinha
*  Class : CS612
* Semester: Fall 2021
*/

#include "pch.h"
#include "Socket.h"

#define INTIAL_BUF_SIZE 4096

Socket::Socket()
{
	buff = new char[INTIAL_BUF_SIZE];
	memset(buff,'\0', INTIAL_BUF_SIZE);
	allocatedSize = INTIAL_BUF_SIZE;
	THRESHOLD = 0.75 * INTIAL_BUF_SIZE;
	curPos = 0;
}

bool Socket::Read(void)
{
	// set timeout to 10 seconds
	timeval timeout = { 10.0L, 0.0L };
	//return value from select call
	int ret = 0;
	clock_t time_req;
	time_req = clock();
	int temp_pos = curPos;
	while (true)
	{
		// Put only the current socket in the fd_set to check of readability
		fd_set fd = { 1,{sock} };
		// check to see readablility of sockets as of now
		if ((ret = select(0, &fd, NULL, NULL, &timeout)) > 0) {
			// new data available; now read the next segment
			int bytes = recv(sock, buff + curPos, allocatedSize - curPos, 0);
			if (bytes == SOCKET_ERROR) {
				printf("\t  Loading... failed with:%d on recv\n", WSAGetLastError());
				break;
			}
			if (bytes == 0)
			{
				// NULL-terminate 
				//buff[curPos] = '\0';
				time_req = clock() - time_req;
				printf("\t  Loading... done in %3.1f ms with %d bytes\n", 1000 * (float)time_req / CLOCKS_PER_SEC, curPos-temp_pos);
				return true; // normal completion
			}
			// adjust where the next recv goes
			curPos += bytes;
			if (allocatedSize - curPos < THRESHOLD)
				// resize buffer; you can use realloc(), HeapReAlloc(), or
				// memcpy the buffer into a bigger array
			{
				allocatedSize *= 2;
				THRESHOLD = allocatedSize * 0.75;
				char *tempBuff = (char *)realloc(buff, allocatedSize);
				if (tempBuff != NULL)
				{
					memcpy(tempBuff, buff, allocatedSize);
					buff = tempBuff;
				}
				else
				{
					printf("\t  Not enough memory to allocate to the receive buffer\n");
					break;
				}
			}

		}
		else if (!ret){
			// report timeout
			printf("\t  Connection took more than 10 seconds while receving data\n");
			break;
		}
		else {
			printf("\t  Connecting on page... failed wtih : %d\n", WSAGetLastError());
			break;
		}
	}
	return false;
}

bool Socket::init_sock(const char *str)
{
	WSADATA wsaData;

	//Initialize WinSock; once per program run
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		printf("\t  WSAStartup error %d\n", WSAGetLastError());
		WSACleanup();
		return false;
	}

	// open a TCP socket
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		printf("\t  socket() generated error %d\n", WSAGetLastError());
		WSACleanup();
		return false;
	}

	// structure used in DNS lookups
	struct hostent* remote;

	// structure for connecting to server
	struct sockaddr_in server;

	// first assume that the string is an IP address
	clock_t time_req;
	time_req = clock();
	DWORD IP = inet_addr(str);
	if (IP == INADDR_NONE)
	{
		// if not a valid IP, then do a DNS lookup
		if ((remote = gethostbyname(str)) == NULL)
		{
			printf("\t  Doing DNS... failed with %d\n", WSAGetLastError());
			return false;
		}
		else // take the first IP address and copy into sin_addr

			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
	}
	else
	{
		// if a valid IP, directly drop its binary version into sin_addr
		server.sin_addr.S_un.S_addr = IP;
	}
	time_req = clock() - time_req;
	printf("\t  Doing DNS ... done in %3.1f ms found %s\n", 1000 * (float)time_req / CLOCKS_PER_SEC, inet_ntoa(server.sin_addr));

	// setup the port # and protocol type
	server.sin_family = AF_INET;
	server.sin_port = htons(80);		// host-to-network flips the byte order

	// connect to the server on port 80
	time_req = clock();
	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		printf("\t  Connecting on page... failed wtih : %d\n", WSAGetLastError());
		return false;
	}
	time_req = clock() - time_req;
	printf("\t* Connecting on page ... done in %3.1f ms\n", 1000 * (float)time_req / CLOCKS_PER_SEC);
	/*printf("Successfully connected to %s (%s) on port %d\n", str, inet_ntoa(server.sin_addr), htons(server.sin_port));*/

	//Send
	send(sock, reqBuffer, strlen(reqBuffer), 0);
	Read();
;
	char* non_http = strstr(buff, "HTTP");
	if (non_http == NULL)
	{
		printf("\t  Loading... failed with non-HTTP header\n");
		return false;
	}
	printf("\t  Verifying header... status code %.*s\n",3, &buff[9]);


	return true;
}

bool Socket::Get(char *str)
{
	bool queryPresent = false;
	bool portPresent = false;
	printf("URL: %s\n", str);	

	int ret_status;
	//Extract fragment if present and replace with /0
	char* fragment = strchr(str, '#');
	if (fragment != NULL)
	{
		ret_status = sprintf(fragBuffer, "%s", fragment + 1);
		fragment = strtok(str, "#");
	}
	else
		ret_status = sprintf(fragBuffer, "%s", "\0");

	//Extract query if present and replace with /0
	char* query = strchr(str, '?');
	if (query != NULL)
	{
		ret_status = sprintf(queryBuffer, "%s", query);
		query = strtok(str, "?");
		queryPresent = true;
	}
	else
		ret_status = sprintf(queryBuffer, "%s", "\0");

	//Find the path using the fact that we can use the identifier ://
	char* uni_ident = strstr(str, "://");
	char* pathpos = strchr(uni_ident + 3, '/');

	if (pathpos != NULL)
	{
		//Extract path first
		ret_status = sprintf(path, "%s", pathpos);
		pathpos = strtok(uni_ident + 3, "/");
	}
	else
		sprintf(path, "%c", '/');

	// //Extract port if found
	char* port_pos = strchr(uni_ident + 3, ':');
	if (port_pos != NULL)
	{
		portPresent = true;
		ret_status = sprintf(port_num, "%s", port_pos + 1);
		port_pos = strtok(uni_ident + 3, ":");
	}
	//Extract host name
	ret_status = sprintf(hostName, "%s", uni_ident + 3);

	//Check for validity of the ports
	if (portPresent)
	{
		//Check if the ports are in the limits
		if((strlen(port_num) == 0) || (atoi(port_num) <= 0 || atoi(port_num) >= 65536))
		{
			printf("\t  Parsing URL... failed with invalid port\n");
			return false;
		}
	}

	printf("\t  Parsing URL... host %s, port %s, request %s%s \n", hostName, port_num, path, queryBuffer);

	//Construct the return status
	if(queryPresent)
		ret_status = sprintf(reqBuffer, "GET %s?%s HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\n Connection:close\r\n\r\n", path, queryBuffer, hostName);
	else
		ret_status = sprintf(reqBuffer, "GET %s HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", path, hostName);
	return true;
}

void Socket::HTMLfileParser(char* str)
{
	char filename[MAX_URL_LEN];
	int ret_status = sprintf(filename, "%s.html", hostName);

	FILE* fPtr;
	fPtr = fopen(filename, "w");

	if (fPtr == NULL)
	{
		/* File not created hence exit */
		printf("\t  Unable to create file.\n");
		return;
	}
	char* retmsg = strstr(buff, "\r\n\r\n");
	if (retmsg != NULL)
		fputs(retmsg, fPtr);

	/* Close file to save file data */
	fclose(fPtr);

	// open html file
	HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	// process errors
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("\t  CreateFile failed with %d\n", GetLastError());
		return;
	}

	// get file size
	LARGE_INTEGER li;
	BOOL bRet = GetFileSizeEx(hFile, &li);
	// process errors
	if (bRet == 0)
	{
		printf("\t  GetFileSizeEx error %d\n", GetLastError());
		return;
	}

	// read file into a buffer
	int fileSize = (DWORD)li.QuadPart;			// assumes file size is below 2GB; otherwise, an __int64 is needed
	DWORD bytesRead;
	// allocate buffer
	char* fileBuf = new char[fileSize];
	// read into the buffer
	bRet = ReadFile(hFile, fileBuf, fileSize, &bytesRead, NULL);
	// process errors
	if (bRet == 0 || bytesRead != fileSize)
	{
		printf("\t  ReadFile failed with %d\n", GetLastError());
		return;
	}

	// done with the file
	CloseHandle(hFile);

	clock_t time_req;
	time_req = clock();
	// create new parser object
	HTMLParserBase* parser = new HTMLParserBase;

	char *baseUrl = str;		// where this page came from; needed for construction of relative links

	int nLinks;
	char* linkBuffer = parser->Parse(fileBuf, fileSize, baseUrl, (int)strlen(baseUrl), &nLinks);

	// check for errors indicated by negative values
	if (nLinks < 0)
		nLinks = 0;

	time_req = clock()-time_req;
	printf("\t+ Parsing page... done in %3.1f ms with %d links\n", 1000 * (float)time_req / CLOCKS_PER_SEC, nLinks);
	Display_Stats();

	delete parser;		// this internally deletes linkBuffer
	delete fileBuf;
}

void Socket::Display_Stats()
{
	printf("\n-------------------------------------------------------------------------------------\n");
	
	char* retmsg = strstr(buff, "\r\n\r\n");
	if (retmsg != NULL)
	{
		printf("%.*s\n", retmsg - buff, buff);
	}
	else
		printf("%s\n", buff);

	return;
}

bool isUnique()
{

}