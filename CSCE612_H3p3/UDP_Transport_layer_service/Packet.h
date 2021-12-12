#pragma once
#include "UDP_Transport.h"

#pragma pack(push,1)
class Packet
{
public:
	int type; // SYN, FIN, data
	int size; // bytes in packet data
	double txTime; // transmission time
	char pkt[MAX_PKT_SIZE]; // packet with header
};
#pragma pack(pop)

