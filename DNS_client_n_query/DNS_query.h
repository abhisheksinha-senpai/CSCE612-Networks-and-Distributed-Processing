#pragma once
#include "pch.h"

/* DNS query types */
#define DNS_A 1 /* name -> IP */
#define DNS_NS 2 /* name server */
#define DNS_CNAME 5 /* canonical name */
#define DNS_PTR 12 /* IP -> name */
#define DNS_HINFO 13 /* host info/SOA */
#define DNS_MX 15 /* mail exchange */
#define DNS_AXFR 252 /* request for zone transfer */
#define DNS_ANY 255 /* all records */

/* flags */
#define DNS_QUERY (0 << 15) /* 0 = query; 1 = response */
#define DNS_RESPONSE (1 << 15)
#define DNS_STDQUERY (0 << 11) /* opcode - 4 bits */
#define DNS_AA (1 << 10) /* authoritative answer */
#define DNS_TC (1 << 9) /* truncated */
#define DNS_RD (1 << 8) /* recursion desired */
#define DNS_RA (1 << 7) /* recursion available */

#define DNS_OK 0 /* success */
#define DNS_FORMAT 1 /* format error (unable to interpret) */
#define DNS_SERVERFAIL 2 /* can’t find authority nameserver */
#define DNS_ERROR 3 /* no DNS entry */
#define DNS_NOTIMPL 4 /* not implemented */
#define DNS_REFUSED 5 /* server refused the query */

#define MAX_DNS_SIZE 512 // largest valid UDP packet

#define MAX_ATTEMPTS 3

#pragma pack(push,1)
struct QueryHeader {
    u_short qType;
    u_short qClass;
};

struct FixedDNSheader {
    u_short TXID;
    u_short flags;
    u_short Nquestions;
    u_short Nanswers;
    u_short Nauthority;
    u_short Naddauth;
};

struct ResourceRecord
{
    u_short type;
    u_short _class;
    u_int ttl;
    u_short data_len;
};
#pragma pack(pop) // restores old packing

void makeDNSQuestion( char* buff, char* host);

unsigned char* readRRwithJumps(unsigned char* buff, unsigned char* reader, unsigned char* headerpos, int size);

unsigned char* printResult(unsigned char* reader);

char* reverseIP(char* host);