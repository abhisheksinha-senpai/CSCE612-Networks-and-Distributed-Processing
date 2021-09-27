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
		if (((float)(time_req1 - time_req) / CLOCKS_PER_SEC) > 2.0)
			break;
		// check to see readablility of sockets as of now
		if ((ret = select(0, &fd, NULL, NULL, &timeout)) != SOCKET_ERROR) {
			// new data available; now read the next segment
			int bytes = recv(sock, buff + curPos, allocatedSize - curPos, 0);
			if (bytes == SOCKET_ERROR) 
				break;

			if (bytes == 0)
			{
				// NULL-terminate 
				buff[curPos] = '\0';
				time_req = clock() - time_req;
				if ((curPos) >= 2097152 && flag == 1)
					return false;

				if ((curPos) >= 16384 && flag == 2)
					return false;
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
					break;
			}

		}
		else if (ret == 0) 
			break;
		else 
			break;
	}
	return false;
}

bool Socket::init_sock(const char* str, int x, LPVOID pParam)
{
	Crawler* cr = ((Crawler*)pParam);
	WSADATA wsaData;

	//Initialize WinSock; once per program run

	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		WSACleanup();
		return false;
	}

	// open a TCP socket
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		WSACleanup();
		return false;
	}

	// structure used in DNS lookups
	struct hostent* remote;

	// structure for connecting to server
	struct sockaddr_in server;

	// first assume that the string is an IP address
	int prevSize = DNSsuccess.size();

	if ((remote = gethostbyname(str)) == NULL)
	{
		WSACleanup();
		return false;
	}
	else // take the first IP address and copy into sin_addr
	{
		DNSsuccess.insert(remote->h_addr);
		if(prevSize< DNSsuccess.size())
			InterlockedAdd(&(cr->DNSLookups), 1);
		memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
	}

	prevSize = seenIPs.size();
	seenIPs.insert(inet_addr(inet_ntoa(server.sin_addr)));
	if (seenIPs.size() > prevSize)
		InterlockedAdd(&cr->IPUnique, 1);

	// setup the port # and protocol type
	server.sin_family = AF_INET;
	server.sin_port = htons(80);		// host-to-network flips the byte order

	// connect to the server on port 80
	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		WSACleanup();
		return false;
	}

	send(sock, reqBuffer, strlen(reqBuffer), 0);

	int ret1 = Read(x);
	if (!ret1)
	{
		WSACleanup();
		return false;
	}
	if (strncmp(&buff[9], "2", 1) == 0 && x==2)
		InterlockedAdd(&cr->robot, 1);

	if (x == 1)
	{
		if (strncmp(&buff[9], "2", 1))
			InterlockedAdd(&cr->http_check2, 1);
		else if (strncmp(&buff[9], "3", 1))
			InterlockedAdd(&cr->http_check3, 1);
		else if (strncmp(&buff[9], "4", 1))
			InterlockedAdd(&cr->http_check4, 1);
		else if (strncmp(&buff[9], "5", 1))
			InterlockedAdd(&cr->http_check4, 1);
		else
			InterlockedAdd(&cr->other, 1);
	}

	return true;
}

bool Socket::Get(char* str, int flag, bool parse)
{
	int ret_status;
	if (strncmp(str, "http://", 7) != 0)
		return false;

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
				//printf("\t  Parsing URL... failed with invalid port\n");
				return false;
			}
		}

		//Extract host name
		hostName = uni_ident + 3;

	}

	//Construct the return status
	if (flag == 1)
	{
		if (queryPresent)
			if (pathPresent)
				ret_status = sprintf(reqBuffer, "HEAD /%s?%s HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", pathpos, query, hostName);
			else
				ret_status = sprintf(reqBuffer, "HEAD /?%s HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", query, hostName);
		else
			if (pathPresent)
				ret_status = sprintf(reqBuffer, "HEAD /%s HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", pathpos, hostName);
			else
				ret_status = sprintf(reqBuffer, "HEAD / HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", hostName);

	}
	else if (flag == 2)
	{
		if (queryPresent)
			if (pathPresent)
				ret_status = sprintf(reqBuffer, "GET /%s?%s HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", pathpos, query, hostName);
			else
				ret_status = sprintf(reqBuffer, "GET /?%s HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", query, hostName);
		else
			if (pathPresent)
				ret_status = sprintf(reqBuffer, "GET /%s HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", pathpos, hostName);
			else
				ret_status = sprintf(reqBuffer, "GET / HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", hostName);
	}
	else
		ret_status = sprintf(reqBuffer, "GET /robots.txt HTTP/1.0\r\nUser-agent: myTAMUcrawler/1.0\r\nHost:%s\r\nConnection:close\r\n\r\n", hostName);

	return true;
}