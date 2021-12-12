#pragma once
#include "pch.h"
#include "ParallelTrace.h"

#define IP_HDR_SIZE 20 /* RFC 791 */
#define ICMP_HDR_SIZE 8 /* RFC 792 */
/* max payload size of an ICMP message originated in the program */
#define MAX_SIZE 65200
/* max size of an IP datagram */
#define MAX_ICMP_SIZE (MAX_SIZE + ICMP_HDR_SIZE)
/* the returned ICMP message will most likely include only 8 bytes
* of the original message plus the IP header (as per RFC 792); however,
* longer replies (e.g., 68 bytes) are possible */
#define MAX_REPLY_SIZE (IP_HDR_SIZE + ICMP_HDR_SIZE + MAX_ICMP_SIZE+40)
/* ICMP packet types */
#define ICMP_ECHO_REPLY 0
#define ICMP_DEST_UNREACH 3
#define ICMP_TTL_EXPIRED 11
#define ICMP_ECHO_REQUEST 8
#define ICMP 1

#define DST_HOST_UNRCH 
#define DST_PRTO_UNRCH
#define DST_PORT_UNRCH
#define FRAG_REQ
#define SRC_RT_FAIL
#define DST_NTWK_UNK
#define DST_HST_UNK
#define SRC_HST_ISO
#define NETWORK_PROBHI
#define HOST_PROBHI
#define NETWORK_UNRCH_TOS
#define HOST_UNRCH_TOS
#define COMM_ADMIN_PROBHI

#define MAX_URL_LENGTH 512
#define DEFAULT_TIMEOUT 500000

#define STATUS_OK 22
#define TIME_OUT -22
#define SEND_ERROR -33
#define RECEIVE_ERROR -44
#define RANDOM_PACKET -55
#define SOCKOPT_ERROR -66
#define STARTUP_ERROR -77
#define CREATESOCK_ERROR -88
#define SOCKASSO_ERROR -99

u_short ip_checksum(u_short* buffer, int size);
std::string getIP(ULONG IP);
UINT Worker_DNS(LPVOID pParam);

extern int hops[40];

struct compareFunc
{
    bool operator()(std::pair<double, int> a, std::pair<double, int> b)
    {
        return a.first > b.first;
    }
};

static inline int64_t GetTicks()
{
	LARGE_INTEGER ticks;
	QueryPerformanceCounter(&ticks);
	return ticks.QuadPart;
}

std::string DNSLookup(ULONG IP);
