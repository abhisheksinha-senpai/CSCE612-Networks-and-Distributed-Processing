#include "pch.h"
#include "ParallelTrace.h"

using namespace std;
double PC_freq_usec=0;

extern std::string* source_cname;
extern ULONG* router_ip;
int hops[40];

ParallelTrace::ParallelTrace()
{
    start_time = GetTicks();
    LARGE_INTEGER lpFrequency;
    QueryPerformanceFrequency(&lpFrequency);
    PC_freq_usec = double(lpFrequency.QuadPart);
 
    //Initialize WinSock; once per program run
    WORD wVersionRequested = MAKEWORD(2, 2);
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        printf("WSAStartup error %d\n", WSAGetLastError());
        throw STARTUP_ERROR;
    }
    /* ready to create a socket */
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == INVALID_SOCKET)
    {
        printf("Unable to create a raw socket: error %d\n", WSAGetLastError());
        throw CREATESOCK_ERROR;
    }
    EstimatedRTT = new double[40]{0};
    retx_count = new int[40]();
    source_ip = new std::string[40]();
    source_cname = new std::string[40];
    sent_time = new int64_t[40]{0};
    Estimated_RTO = new double[40]{ DEFAULT_TIMEOUT };
    router_ip = new ULONG[40]{ 0 };
    ICMP_reply_buf = new char[MAX_REPLY_SIZE];
    ICMP_send_buf = new char[MAX_ICMP_SIZE];
    received = new bool[40]{ false };
    rcv_type = new int[40]{ 0 };
    rcv_code = new int[40]{ 0 };
    socketReceiveReady = CreateEvent(NULL, false, false, NULL);
    
    server.sin_family = AF_INET;
    finish_index = 0;
}

void ParallelTrace::Traceroute(ULONG IP)
{
    server.sin_addr.S_un.S_addr = IP;
    for (int i = 1; i < 31; i++)
    {
        ICMP_Tx(i);
    }
    try {
        int status = WSAEventSelect(sock, socketReceiveReady, FD_READ);
        if (status == SOCKET_ERROR)           
            throw SOCKASSO_ERROR;

    }
    catch (int wsaError)
    {
        printf("Socket association error with error code %d\n", WSAGetLastError());
        throw wsaError;
    }
   

    while (!future_retx.empty())
    {
        double currTime = 1000000 * (GetTicks() - start_time) / PC_freq_usec;

        double timeout = max(future_retx.top().first - currTime,0);
        int ret = WaitForSingleObject(socketReceiveReady, timeout / 1000);
        switch (ret)
        {
        case WAIT_TIMEOUT:
        {
            try
            {
                int pack_num = future_retx.top().second;
                future_retx.pop();
                if (retx_count[pack_num] < 3)
                {
                    MakePacket(pack_num);
                    SendPacket(pack_num);
                }
            }
            catch (int wsaError)
            {
                printf("Failed Sending packet due to WSA error %d\n", WSAGetLastError());
                throw wsaError;
            }
            break;
        }
        case WAIT_OBJECT_0:
        {
            try
            {
                ReceivePacket();
            }
            catch (int wsaError)
            {
                printf("Failed Receiving packet due to WSA error %d\n", WSAGetLastError());
                throw wsaError;
            }
            break;
        }
        }
        if (finished)
            break;
    }
    while (!finished)
    {
        int ret = WaitForSingleObject(socketReceiveReady, 30000);
        if (ret == WAIT_TIMEOUT)
            break;
        try
        {
            ReceivePacket();
        }
        catch (int wsaError)
        {
            printf("Failed Receiving packet due to WSA error %d\n", WSAGetLastError());
            throw wsaError;
        }
    }
    int count = 30;
    while (count > -1)
    {
        WaitForMultipleObjects(30, workerthread_DNS, false, INFINITE);
        count--;
    }
    int i = 0;
    for (i = 0; i < 40; i++)
        CloseHandle(workerthread_DNS[i]);
    printTraceRoute();
    
    
}

void ParallelTrace::ICMP_Tx(int hopNumber)
{
    MakePacket(hopNumber);
    try
    {
        SendPacket(hopNumber);
    }
    catch (int wsaError)
    {
        printf("Failed Sending packet due to WSA error %d\n", WSAGetLastError());
        throw wsaError;
    }        
}


ULONG ParallelTrace::GetNextIP()
{
    sockaddr_in server_T;
    std::string target = websites.front();
    websites.pop();
    DWORD IP_add = inet_addr(target.c_str());
    hostent* remote;
    server_T.sin_family = AF_INET;
    if (IP_add == INADDR_NONE)
    {
        if ((remote = gethostbyname(target.c_str())) == NULL)
        {
            printf("gethostbyname() failed: %d\n", WSAGetLastError());
            exit(EXIT_FAILURE);
        }
        else
            memcpy((char*)&(server_T.sin_addr), remote->h_addr, remote->h_length);
    }
    else
        server_T.sin_addr.S_un.S_addr = IP_add;

    printf("Tracerouting to %s...\n", inet_ntoa(server_T.sin_addr));
    targetIP.append(inet_ntoa(server_T.sin_addr));
    return server_T.sin_addr.S_un.S_addr;
}

void ParallelTrace::printTraceRoute()
{
    hostent* remoteHost;
    for (int i = 1; i <=finish_index; i++)
    {
        if (rcv_type[i] == ICMP_DEST_UNREACH)
        {
            printf("Destination Unreachable with code %d",rcv_code[i]);
            return;
        }
        if (received[i] == true)
        {
            if (source_cname[i] == "")
                source_cname[i] = "<no DNS entry>";
            printf("%d\t%-4s\t%-4s\t%.3lf ms\t(%d)\n", i, source_cname[i].c_str(), source_ip[i].c_str(), EstimatedRTT[i] / 1000, retx_count[i]);
        }
        else
            printf("*\n");
    }
}

void ParallelTrace::MakePacket(int hopNumber)
{
    ICMPHeader* icmp = (ICMPHeader*)ICMP_send_buf;
    icmp->type = (ICMP_ECHO_REQUEST);
    icmp->code = 0;
    icmp->id = ((u_short)GetCurrentProcessId());
    icmp->checksum = 0;
    icmp->seq = (hopNumber);
    int packet_size = sizeof(ICMPHeader); // 8 bytes
    icmp->checksum = ip_checksum((u_short*)ICMP_send_buf, packet_size);
}

int ParallelTrace::SendPacket(int hopNumber)
{
    int ttl = hopNumber;
    if (setsockopt(sock, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl)) == SOCKET_ERROR)
        throw SOCKOPT_ERROR;
    sent_time[hopNumber] = 1000000*(GetTicks() - start_time)/ PC_freq_usec;
    int status = sendto(sock, (char*)ICMP_send_buf, sizeof(ICMPHeader), 0, (struct sockaddr*)&(server), sizeof(server));
    if (status == SOCKET_ERROR)
        throw SEND_ERROR;
    DynamicTimeout(hopNumber);
    //printf("%lf\t%d\n", Estimated_RTO[hopNumber], hopNumber);
    future_retx.push(std::make_pair(sent_time[hopNumber] + Estimated_RTO[hopNumber], hopNumber));
    hops[hopNumber] = hopNumber;
    retx_count[hopNumber]++;
    return STATUS_OK;
}

int ParallelTrace::ReceivePacket()
{
    IPHeader* router_ip_hdr = (IPHeader*)ICMP_reply_buf;
    //printf("%d  %d", (router_ip_hdr->h_len), ntohs(router_ip_hdr->h_len));
    ICMPHeader* router_icmp_hdr = (ICMPHeader*)(router_ip_hdr+1);// (4 * (router_ip_hdr->h_len) + (char*)router_ip_hdr);
    IPHeader* orig_ip_hdr = (IPHeader*)(router_icmp_hdr + 1);
    ICMPHeader* orig_icmp_hdr = (ICMPHeader*)(orig_ip_hdr + 1);
    sockaddr_in response_addr;
    int sz = sizeof(response_addr);

    int response = recvfrom(sock, (char*)ICMP_reply_buf, MAX_REPLY_SIZE, 0, reinterpret_cast<struct sockaddr*>(&response_addr), &sz);
        
    if (response == SOCKET_ERROR)
        throw RECEIVE_ERROR;

    if ((orig_ip_hdr->proto == ICMP) && (orig_icmp_hdr->id == GetCurrentProcessId()))
    {
        //if (!received[(orig_icmp_hdr->seq)])
        {
            rcv_type[(orig_icmp_hdr->seq)] = router_icmp_hdr->type;
            rcv_code[(orig_icmp_hdr->seq)] = router_icmp_hdr->code;
            if ((router_icmp_hdr->type == ICMP_ECHO_REPLY) || (router_icmp_hdr->type == ICMP_TTL_EXPIRED))
            {
                EstimatedRTT[(orig_icmp_hdr->seq)] = 1000000 * (GetTicks() - start_time) / PC_freq_usec - sent_time[(orig_icmp_hdr->seq)];
                router_ip[(orig_icmp_hdr->seq)] = router_ip_hdr->source_ip;
                source_ip[(orig_icmp_hdr->seq)] = getIP(router_ip_hdr->source_ip);

                received[(orig_icmp_hdr->seq)] = true;

               if(!workerQuits[(orig_icmp_hdr->seq)])
                    workerthread_DNS[(orig_icmp_hdr->seq)] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Worker_DNS, &hops[(orig_icmp_hdr->seq)], 0, NULL);
                workerQuits[(orig_icmp_hdr->seq)] = true;
                finish_index = max(finish_index, orig_icmp_hdr->seq);
                if (source_ip[(orig_icmp_hdr->seq)] == targetIP || orig_icmp_hdr->seq == 31)
                {
                    finish_index = (orig_icmp_hdr->seq);
                    finished = true;
                }
            }
        }
    }
}

ParallelTrace::~ParallelTrace()
{
    delete[] EstimatedRTT;
    delete[] retx_count;
    delete[] source_ip;
    delete[] source_cname;
    delete[] sent_time;
    delete[] Estimated_RTO;
    delete[] router_ip;
    delete[] ICMP_reply_buf;
    delete[] ICMP_send_buf;
    delete[] received;
    delete[] rcv_type;
    delete[] rcv_code;
    closesocket(sock);
    // some cleanup 
    WSACleanup();
}
void ParallelTrace::DynamicTimeout(int hopNumber)
{
    int next = 0;
    if (received[hopNumber - 1] == true && received[hopNumber + 1] == true)
        Estimated_RTO[hopNumber] = min(2 * (EstimatedRTT[hopNumber - 1] + EstimatedRTT[hopNumber + 1]) / 2, DEFAULT_TIMEOUT);
    else if (received[hopNumber - 1] == false && received[hopNumber + 1] == true)
        Estimated_RTO[hopNumber] = min(2 * EstimatedRTT[hopNumber + 1], DEFAULT_TIMEOUT);
    else if (received[hopNumber - 1] == true && received[hopNumber + 1] == false)
        Estimated_RTO[hopNumber] = min(2 * EstimatedRTT[hopNumber - 1] + 100000, DEFAULT_TIMEOUT);
    else
    {
        for (int i = hopNumber + 1; i < 32; i++)
        {
            if (received[i] == true)
            {
                next = i;
                break;
            }
        }

        if (next > 0)
            Estimated_RTO[hopNumber] = min(2 * EstimatedRTT[next], DEFAULT_TIMEOUT);
        else
            Estimated_RTO[hopNumber] = DEFAULT_TIMEOUT;
    }
}
