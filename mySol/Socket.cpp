/* Author: Abhishek Sinha
*  Class : CS612
* Semester: Fall 2021
*/

#include "pch.h"
#include "Socket.h"

#define INTIAL_BUF_SIZE 4096

Socket::Socket()
{
	buff = (char*)(malloc(INTIAL_BUF_SIZE));
	//memset(buff,'\0', INTIAL_BUF_SIZE);
	allocatedSize = INTIAL_BUF_SIZE;
	THRESHOLD = 0.75 * INTIAL_BUF_SIZE;
	curPos = 0;
}

bool Socket::Read(int flag)
{
	// set timeout to 10 seconds
	timeval timeout = { 10.0L, 0.0L };
	//return value from select call
	int ret = 0;
	clock_t time_req;
	time_req = clock();
	allocatedSize = INTIAL_BUF_SIZE;
	THRESHOLD = 0.75 * INTIAL_BUF_SIZE;
	curPos = 0;
	while (true)
	{
		// Put only the current socket in the fd_set to check of readability
		fd_set fd = { 1,{sock} };
		clock_t time_req1 = clock();
		if (((float)(time_req1 - time_req) / CLOCKS_PER_SEC) > 10.0)
		{
			printf("\t  Connection took more than 10 seconds while receving data\n");
			break;
		}
		// check to see readablility of sockets as of now
		if ((ret = select(0, &fd, NULL, NULL, &timeout)) != SOCKET_ERROR) {
			// new data available; now read the next segment
			int bytes = recv(sock, buff + curPos, allocatedSize - curPos, 0);
			if (bytes == SOCKET_ERROR) {
				printf("\t  Loading... failed with:%d on recv\n", WSAGetLastError());
				break;
			}
			if (bytes == 0)
			{
				// NULL-terminate 
				buff[curPos] = '\0';
				time_req = clock() - time_req;
				if ((curPos) >= 2097152 && flag == 1)
				{
					printf("\t  Loading... failed with exceeding max\n");
					return false;
				}
				if ((curPos) >= 16384 && flag == 2)
				{
					printf("\t  Loading... failed with exceeding max\n");
					return false;
				}
				printf("\t  Loading... done in %3.1f ms with %d bytes\n", 1000 * (float)time_req / CLOCKS_PER_SEC, curPos);
				return true; // normal completion
			}
			// adjust where the next recv goes
			curPos += bytes;
			if (allocatedSize - curPos < THRESHOLD)
				// resize buffer; you can use realloc(), HeapReAlloc(), or
				// memcpy the buffer into a bigger array
			{
				//int old_size = allocatedSize;
				allocatedSize *= 2;
				THRESHOLD = allocatedSize * 0.75;
				buff = (char*)realloc(buff, allocatedSize);
				if (buff == NULL)
				{
					printf("\t  Not enough memory to allocate to the receive buffer\n");
					break;
				}
			}

		}
		else if (ret == 0) {
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

bool Socket::init_sock(const char* str, int x)
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

	if ((remote = gethostbyname(str)) == NULL)
	{
		printf("\t  Doing DNS... failed with %d\n", WSAGetLastError());
		return false;
	}
	else // take the first IP address and copy into sin_addr
		memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);

	time_req = clock() - time_req;

	if (x == 2)
	{
		printf("\t  Doing DNS ... done in %3.1f ms found %s\n", 1000 * (float)time_req / CLOCKS_PER_SEC, inet_ntoa(server.sin_addr));

		int prevSize = seenIPs.size();
		seenIPs.insert(inet_addr(inet_ntoa(server.sin_addr)));
		if (seenIPs.size() <= prevSize)
		{
			printf("\t  Checking IP uniqueness... failed\n");
			return false;
		}
		else
			printf("\t  Checking IP uniqueness... passed\n");
	}

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
	if (x == 1)
	{
		time_req = clock() - time_req;
		printf("\t* Connecting on page ... done in %3.1f ms\n", 1000 * (float)time_req / CLOCKS_PER_SEC);
		send(sock, reqBuffer, strlen(reqBuffer), 0);
		//printf("Successfully connected to %s (%s) on port %d\n", str, inet_ntoa(server.sin_addr), htons(server.sin_port));

	}
	if (x == 2)
	{
		time_req = clock() - time_req;
		printf("\t* Connecting on robots ... done in %3.1f ms\n", 1000 * (float)time_req / CLOCKS_PER_SEC);
		send(sock, reqBuffer, strlen(reqBuffer), 0);
	}

	int ret1 = Read(x);
	if (!ret1)
		return false;

	/*if (x == 1)
	printf("\n%s\n", buff);*/

	printf("\t  Verifying header... status code %.*s\n", 3, &buff[9]);

	return true;
}

bool Socket::Get(char* str, int flag, bool parse)
{
	int ret_status;
	if (strncmp(str, "http://", 7) != 0)
	{
		printf("\t  Loading... failed with non-HTTP header\n");
		return false;
	}
	if (parse)
	{
		queryPresent = false;
		portPresent = false;
		pathPresent = false;
		hostName = NULL;
		port_pos = NULL;
		pathpos = NULL;

		//Extract query if present and replace with /0
		query = strchr(str, '?');
		if (query == NULL)
			queryPresent = false;
		else
		{
			*query = 0;
			query++;
			queryPresent = true;
		}


		//Find the path using the fact that we can use the identifier ://
		char* uni_ident = strstr(str, "://");
		pathpos = strchr(uni_ident + 3, '/');

		if (pathpos != NULL)
		{
			//Extract path first
			*pathpos = 0;
			pathpos++;
			pathPresent = true;
			if (strlen(pathpos) == 0)
				pathPresent = false;
		}
		else
			pathPresent = false;

		// //Extract port if found
		char* port_pos = strchr(uni_ident + 3, ':');
		if (port_pos != NULL)
		{
			portPresent = true;
			*port_pos = 0;
			port_pos++;
		}
		else
			portPresent = false;

		//Check for validity of the ports
		if (portPresent)
		{
			//Check if the ports are in the limits
			if ((strlen(port_pos) == 0) || (atoi(port_pos) <= 0 || atoi(port_pos) >= 65536))
			{
				printf("\t  Parsing URL... failed with invalid port\n");
				return false;
			}
		}

		//Extract host name
		hostName = uni_ident + 3;

		if (flag == 3)
			printf("Parsing URL... host %s, port 80 \n", hostName);
	}

	//Construct the return status
	if (flag == 1)
	{
		if (queryPresent)
			if (pathPresent)
				ret_status = sprintf(reqBuffer, "HEAD /%s?%s HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", pathpos, query, hostName);
			else
				ret_status = sprintf(reqBuffer, "HEAD /?%s HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", query, hostName);
		else
			if (pathPresent)
				ret_status = sprintf(reqBuffer, "HEAD /%s HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", pathpos, hostName);
			else
				ret_status = sprintf(reqBuffer, "HEAD / HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", hostName);

	}
	else if (flag == 2)
	{
		if (queryPresent)
			if (pathPresent)
				ret_status = sprintf(reqBuffer, "GET /%s?%s HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", pathpos, query, hostName);
			else
				ret_status = sprintf(reqBuffer, "GET /?%s HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", query, hostName);
		else
			if (pathPresent)
				ret_status = sprintf(reqBuffer, "GET /%s HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", pathpos, hostName);
			else
				ret_status = sprintf(reqBuffer, "GET / HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", hostName);
	}
	else
		ret_status = sprintf(reqBuffer, "GET /robots.txt HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", hostName);

	//ret_status = sprintf(reqBuffer, "HEAD%s/robots.txt HTTP/1.0\r\n\User-agent: myTAMUcrawler/1.0\r\nHost:%s\r\n Connection:close\r\n\r\n"," ");

	return true;
}

void Socket::HTMLfileParser(char* str)
{
	char filename[MAX_URL_LEN];
	int ret_status = sprintf(filename, "%s.html", NULL);

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

	char* baseUrl = str;		// where this page came from; needed for construction of relative links

	int nLinks;
	char* linkBuffer = parser->Parse(fileBuf, fileSize, baseUrl, (int)strlen(baseUrl), &nLinks);

	// check for errors indicated by negative values
	if (nLinks < 0)
		nLinks = 0;

	time_req = clock() - time_req;
	printf("\t+ Parsing page... done in %3.1f ms with %d links\n", 1000 * (float)time_req / CLOCKS_PER_SEC, nLinks);
	//Display_Stats();

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

bool Socket::isUnique()
{
	return false;
}