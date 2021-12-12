#include "pch.h"

std::string* source_cname;
ULONG* router_ip;
extern int hops[];

u_short ip_checksum(u_short* buffer, int size)
{
	u_long cksum = 0;
	/* sum all the words together, adding the final byte if size is odd */
	while (size > 1)
	{
		cksum += *buffer++;
		size -= sizeof(u_short);
	}
	if (size)
		cksum += *(u_char*)buffer;
	/* add carry bits to lower u_short word */ 
	cksum = (cksum >> 16) + (cksum & 0xffff);	
	/* return a bitwise complement of the resulting mishmash */
	return (u_short) (~cksum);
}

std::string DNSLookup(ULONG IP)
{
    hostent* hp;
    in_addr ip = {0};
    ip.S_un.S_addr = IP;
    hp = gethostbyaddr((const char*)&ip, sizeof ip, AF_INET);
    if (hp == NULL)
        return "<no DNS entry>";
    else
    {
        std::string temp;
        temp.append(hp->h_name);
        return temp;
    }
}

std::string getIP(ULONG IP)
{
    unsigned char octet[4];
    for (int i = 0; i < 4; i++)
        octet[i] = ((IP >> 8 * i) & 0xFF);
    char temp[20];
    sprintf(temp, "%d.%d.%d.%d", octet[0], octet[1], octet[2], octet[3]);
    std::string str;
    str.append(temp);
    return str;
        
}

UINT Worker_DNS(LPVOID pParam)
{
    int* addr = (int*)pParam;
    //printf("%d\n", *addr);
    source_cname[*addr] = DNSLookup(router_ip[*addr]);

    return 0;
}