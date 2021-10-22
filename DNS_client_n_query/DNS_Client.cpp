#include "pch.h"
#include "DNS_Client.h"

DNS_Client::DNS_Client()
{
    WSADATA wsaData;
    //Initialize WinSock; once per program run
    WORD wVersionRequested = MAKEWORD(2, 2);
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        printf("WSAStartup error %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    memset(RR, 0, MAX_DNS_SIZE);
    memset(myhost, 0, MAX_DNS_SIZE);
    memset(query, 0, MAX_DNS_SIZE);
}

bool DNS_Client::query_generator(int qt, char* host)
{
    bool ret_status = false;
    memcpy(myhost, host, strlen(host) + 1);
    query_size = strlen(myhost) + 2 + sizeof(FixedDNSheader) + sizeof(QueryHeader);
    FixedDNSheader* fdh = (FixedDNSheader*)query;
    QueryHeader* qh = (QueryHeader*)(query + query_size - sizeof(QueryHeader));
    fdh->TXID = htons(0X1234);
    fdh->flags = htons(DNS_QUERY | DNS_STDQUERY | DNS_RD);
    fdh->Nquestions = htons(1);
    fdh->Nanswers = 0;
    fdh->Nauthority = 0;
    fdh->Naddauth = 0;
    makeDNSQuestion(&query[sizeof(FixedDNSheader)], host);
    qh->qClass = htons(1);
    switch (qt) {
    case DNS_A:
        qh->qType = htons(DNS_A);
        break;
    case DNS_PTR: 
        qh->qType = htons(DNS_PTR);
        break;
    };
    printf("%-12s: %s  type %hu TXID 0X%04x\n", "Query", myhost, ntohs(qh->qType), ntohs(fdh->TXID));
//printf("tr %-12x\n", ntohs(*((unsigned short*)query + 1)));
    return ret_status;
}

int DNS_Client::query_type_identifier(char* kworgs)
{
    unsigned long  ret = inet_addr(kworgs);
    if (ret == INADDR_NONE)
        return DNS_A;
    else
        return DNS_PTR;
}

void DNS_Client::UDP_sender(char* dns)
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(0);
    if (bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
    {
        printf("Error while binding: %d\n", WSAGetLastError());
        errorflag = true;
        closesocket(sock);
        return;
    }

    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.S_un.S_addr = inet_addr(dns); // server’s IP
    remote.sin_port = htons(53); // DNS port on server
    if (sendto(sock, query, query_size, 0, (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR)
    {
        printf("Error while sending: %d\n", WSAGetLastError());
        WSACleanup();
        errorflag = true;
        closesocket(sock);
        return;
    }
    return;
}

void DNS_Client::response_parser(char* dns)
{
    int count = 0;
    sockaddr_in response_addr;
    int sz = sizeof(response_addr);
    memset(&response_addr, 0, sizeof(response_addr));
    clock_t start, end;

    printf("%-12s%s%s\n", "Server", ": ", dns);
    printf("********************************\n");

    while (count++ < MAX_ATTEMPTS)
    {
        struct sockaddr_in temp;
        // send request to the server
        UDP_sender(dns);
        // get ready to receive

        timeval tp = { 10,0 };
        fd_set fd;
        FD_ZERO(&fd); // clear the set
        FD_SET(sock, &fd); // add your socket to the set
        start = clock();
        int available = select(0, &fd, NULL, NULL, &tp);
        if (available > 0)
        {
            int response = recvfrom(sock, RR, MAX_DNS_SIZE, 0, reinterpret_cast<struct sockaddr*>(&response_addr), &sz);
            //printf("%X\n", ntohs(*RR));
            if (response == SOCKET_ERROR) {
                printf("Error while receving: %d\n", WSAGetLastError());
                WSACleanup();
                errorflag = true;
                closesocket(sock);
                return;
            }
            end = clock();
            // parse the response
            if ((unsigned long)(response_addr.sin_addr.S_un.S_addr) != (unsigned long)(remote.sin_addr.S_un.S_addr) || response_addr.sin_port != remote.sin_port)
            {
                printf("\n++\t Invalid reply: return address not same as host\n");
                WSACleanup();
                errorflag = true;
                closesocket(sock);
                return;
            }

            if (*((unsigned char*)RR + 2) & DNS_RA)
            {
                FixedDNSheader* fdh = (FixedDNSheader*)query;
                fdh->flags |= (DNS_RA | DNS_RESPONSE);
            }
            else
            {
                FixedDNSheader* fdh = (FixedDNSheader*)query;
                fdh->flags |= (DNS_RESPONSE);
            }
            unsigned char* res = (unsigned char*)RR;// (unsigned char*)strstr((char*)RR, (char*)query);
            unsigned char* reader = res;

            if (response < sizeof(FixedDNSheader))
            {
                printf("\n++\tinvalid reply: smaller than fixed header\n");
                WSACleanup();
                errorflag = true;
                closesocket(sock);
                return;
            }
            auto dns_header = reinterpret_cast<FixedDNSheader*>(RR);
            auto qname = &RR[sizeof(FixedDNSheader)];
            int pos = 0;
            // break from the loop
            printf("Attempt %d with %d bytes... response in %d ms with %d bytes\n", count, query_size, 1000 * (int)((double)(end - start)) / CLOCKS_PER_SEC, response);
            printf("\tTXID 0X%x, flags 0X%x, questions %hu, answers %hu, authority %hu, additional %hu\n", ntohs(dns_header->TXID), ntohs(dns_header->flags), ntohs(dns_header->Nquestions), ntohs(dns_header->Nanswers), ntohs(dns_header->Nauthority), ntohs(dns_header->Naddauth));

            FixedDNSheader* fdh = (FixedDNSheader*)query;
            if (ntohs(dns_header->TXID) != ntohs(fdh->TXID))
            {
                printf("\t++ invalid reply: TXID mismatch, sent 0x%04x, received 0x%04x\n", ntohs(fdh->TXID), ntohs(dns_header->TXID));
                return;
            }
            int code = ntohs(dns_header->flags) & 0x000F;
            if (code == 0)
                printf("\tsucceeded with Rcode = %d\n", code);
            else
            {
                printf("\tfailed with Rcode = %d\n", code);
                errorflag = true;
                closesocket(sock);
                WSACleanup();
                return;
            }

            res = res + sizeof(FixedDNSheader);
            printf("\t------------[questions] ------------\n");
            for (int i = 0; i < ntohs(dns_header->Nquestions); i++)
            {
                printf("\t\t");
                res = printResult(res);
                printf(" ");
                //print the qclass and qtype
                QueryHeader* temp_qh = (QueryHeader*)res;
                printf("type %hu class %hu\n", ntohs(temp_qh->qType), ntohs(temp_qh->qClass));
                res = res + sizeof(QueryHeader);
            }

            /*bool authority_gone = false;
            bool answer_gone = false;

            bool header_auth = false;
            bool header_add = false;
            bool inauth = false;
            ResourceRecord* rr;
            if (ntohs(dns_header->Nanswers) > 0)
                printf("\t------------[answers] ------------\n");
            unsigned char* prev = 0;
            int count_value_A = 0, count_value_Auth = 0, count_value_Add = 0;
            bool flag = false;
            int count_total = ntohs(dns_header->Naddauth) + ntohs(dns_header->Nanswers) + ntohs(dns_header->Nauthority);
            for(int u=0;u<count_total;u++)
            {
                if (ntohs(dns_header->Nanswers) == count_value_A)
                    answer_gone = true;
                else if (ntohs(dns_header->Nanswers) == 0)
                    answer_gone = true;
                else if (ntohs(dns_header->Nanswers) > 0 && !answer_gone)
                    count_value_A++;

                
                if (answer_gone && ntohs(dns_header->Nauthority) == 0)
                    authority_gone = true;
                else if (answer_gone && ntohs(dns_header->Nauthority) == count_value_Auth)
                    authority_gone = true;
                else if (answer_gone && ntohs(dns_header->Nauthority) > 0 && !authority_gone)
                    count_value_Auth++;

                if (answer_gone && authority_gone && ntohs(dns_header->Naddauth) > 0)
                    count_value_Add++;

                if (answer_gone && !authority_gone && !header_auth && ntohs(dns_header->Nauthority) > 0)
                {
                    header_auth = true;
                    printf("\t------------[authority] ------------\n");
                }

                if (answer_gone && authority_gone && !header_add && ntohs(dns_header->Naddauth) > 0)
                {
                    header_add = true;
                    printf("\t------------[additional] ------------\n");
                }

                if (answer_gone && count_value_A < ntohs(dns_header->Nanswers))
                {
                    printf("\n++\tinvalid section: not enough records\n");
                    errorflag = true;
                    closesocket(sock);
                    WSACleanup();
                    return;
                }
                if (authority_gone && count_value_Auth < ntohs(dns_header->Nauthority))
                {
                    printf("\n++\tinvalid section: not enough records\n");
                    errorflag = true;
                    closesocket(sock);
                    WSACleanup();
                    return;
                }

                printf("\t");
                res = readRRwithJumps((unsigned char*)RR, (unsigned char*)res, reader, response);
                
                if ((res + sizeof(ResourceRecord) - (unsigned char*)RR > response))
                {
                    printf("\n++\tinvalid record: truncated RR answer header\n");
                    errorflag = true;
                    closesocket(sock);
                    WSACleanup();
                    return;
                }
                

                rr = (ResourceRecord*)(res);
                res = res + sizeof(ResourceRecord);
                switch ((int)(ntohs(rr->type)))
                {
                case DNS_A:
                    printf("A ");
                    break;
                case DNS_PTR:
                    printf("PTR ");
                    break;
                case DNS_NS:
                    inauth = true;
                    printf("NS ");
                    break;
                case DNS_CNAME:
                    printf("CNAME ");
                    break;
                };
                if (ntohs(rr->type) == DNS_A)
                {
                    if (res + (int)ntohs(rr->data_len) - (unsigned char*)RR > response)
                    {
                        printf("\n++\tinvalid record:RR value length stretches the answer beyond packet\n");
                        errorflag = true;
                        closesocket(sock);
                        WSACleanup();
                        return;
                    }

                    for (int j = 0; j < 4; j++)
                        printf("%d.", 16 * (unsigned char(res[j]) >> 4) + (unsigned char(res[j]) & 0x0f));
                    printf(" TTL = %d\n", ntohl(rr->ttl));
                    res = res + 4;
                }
                else if ((int)ntohs(rr->type) == DNS_PTR || (int)ntohs(rr->type) == DNS_NS || (int)ntohs(rr->type) == DNS_CNAME)
                {
                    if (res + (int)ntohs(rr->type) - (unsigned char*)RR > response)
                    {
                        printf("\n++\tinvalid record:RR value length stretches the answer beyond packet\n");
                        errorflag = true;
                        closesocket(sock);
                        WSACleanup();
                        return;
                    }
                    res = readRRwithJumps((unsigned char*)RR, (unsigned char*)res, reader, response);
                    printf(" TTL = %d\n", ntohl(rr->ttl));
                    
                }
                prev = res;
            }
            if (count_value_Add < ntohs(dns_header->Naddauth))
            {
                printf("\n++\tinvalid section: not enough records\n");
                errorflag = true;
                closesocket(sock);
                WSACleanup();
                return;
            }
            break;*/
            ResourceRecord* rr;
            if (ntohs(dns_header->Nanswers) > 0)
            {
                printf("\t------------[answers] ------------\n");
                for (int i = 0; i < ntohs(dns_header->Nanswers); i++)
                {
                    if (res - (unsigned char*)RR >= response)
                    {
                        printf("++\tinvalid section: not enough records\n");
                        return;
                    }

                    printf("\t\t");
                    res = readRRwithJumps((unsigned char* )RR,(unsigned char*)res, reader, response);


                    if (res + sizeof(ResourceRecord) - (unsigned char*)RR > response)
                    {
                        printf("++\tinvalid record: truncated RR answer header\n");
                        errorflag = true;
                        closesocket(sock);
                        WSACleanup();
                        return;
                    }

                    rr = (ResourceRecord*)(res);
                    res = res + sizeof(ResourceRecord);
                    switch ((int)(ntohs(rr->type)))
                    {
                        case DNS_A:
                            printf("A ");
                            break;
                        case DNS_PTR:
                            printf("PTR ");
                            break;
                        case DNS_NS:
                            printf("NS ");
                            break;
                        case DNS_CNAME:
                            printf("CNAME ");
                            break;
                    };
                    if (ntohs(rr->type) == DNS_A)
                    {
                        if (res + (int)ntohs(rr->data_len) - (unsigned char*)RR > response)
                        {
                            printf("\t ++ invalid record: RR value length beyond packet\n");
                            errorflag = true;
                            closesocket(sock);
                            WSACleanup();
                            return;
                        }

                        for(int j=0;j<4;j++)
                            printf("%d.", 16 * (unsigned char(res[j]) >> 4) + (unsigned char(res[j]) & 0x0f));
                        printf(" TTL = %d\n", ntohl(rr->ttl));
                        res = res + 4;
                    }
                    else if ((int)ntohs(rr->type) == DNS_PTR || (int)ntohs(rr->type) == DNS_NS || (int)ntohs(rr->type) == DNS_CNAME)
                    {
                        if (res + (int)ntohs(rr->type) - (unsigned char*)RR > response)
                        {
                            printf("\t ++ invalid record: RR value length beyond packet\n");
                            errorflag = true;
                            closesocket(sock);
                            WSACleanup();
                            return;
                        }
                        res = readRRwithJumps((unsigned char*)RR, (unsigned char*)res, reader, response);
                        printf(" TTL = %d\n", ntohl(rr->ttl));
                    }
                }
            }

            if (ntohs(dns_header->Nauthority) > 0)
            {

                printf("\t------------[authority] ------------\n");
                for (int i = 0; i < ntohs(dns_header->Nauthority); i++)
                {
                    if (res - (unsigned char*)RR >= response)
                    {
                        printf("++\tinvalid section: not enough records\n");
                        WSACleanup();
                        return;
                    }
                    printf("\t\t");
                    res = readRRwithJumps((unsigned char*)RR, (unsigned char*)res, reader, response);
                    rr = (ResourceRecord*)(res);
                    res = res + sizeof(ResourceRecord);

                    if (res + sizeof(ResourceRecord) - (unsigned char*)RR > response)
                    {
                        printf("++\tinvalid record: truncated RR answer header\n");
                        errorflag = true;
                        closesocket(sock);
                        WSACleanup();
                        return;
                    }

                    if (ntohs(rr->type) == DNS_A)
                    {
                        if (res + (int)ntohs(rr->data_len) - (unsigned char*)RR > response)
                        {
                            printf("\t ++ invalid record: RR value length beyond packet\n");
                            errorflag = true;
                            closesocket(sock);
                            WSACleanup();
                            return;
                        }
                        for (int j = 0; j < 4; j++)
                            printf("%d.", 16 * (unsigned char(res[j]) >> 4) + (unsigned char(res[j]) & 0x0f));
                        printf(" TTL = %d\n", ((int)ntohs(rr->ttl)));
                        res = res + 4;
                    }
                    else if ((int)ntohs(rr->type) == DNS_PTR || (int)ntohs(rr->type) == DNS_NS || (int)ntohs(rr->type) == DNS_CNAME)
                    {
                        switch ((int)(ntohs(rr->type)))
                        {
                        case DNS_PTR:
                            printf("PTR ");
                            break;
                        case DNS_NS:
                            printf("NS ");
                            break;
                        case DNS_CNAME:
                            printf("CNAME ");
                            break;
                        };
                        if (res + (int)ntohs(rr->data_len) - (unsigned char*)RR > response)
                        {
                            printf("\t ++ invalid record: RR value length beyond packet\n");
                            errorflag = true;
                            closesocket(sock);
                            WSACleanup();
                            return;
                        }
                        res = readRRwithJumps((unsigned char*)RR, (unsigned char*)res, reader, response);
                        printf(" TTL = %d\n", ntohs(rr->ttl));
                    }
                }
            }

            if (ntohs(dns_header->Naddauth) > 0)
            {
                printf("\t------------[additionals] ------------\n");
                for (int i = 0; i < ntohs(dns_header->Naddauth); i++)
                {
                    if (res - (unsigned char*)RR >= response)
                    {
                        printf("++\tinvalid section: not enough records\n");
                        errorflag = true;
                        closesocket(sock);
                        WSACleanup();
                        return;
                    }
                    printf("\t\t");
                    res = readRRwithJumps((unsigned char*)RR, (unsigned char*)res, reader, response);
                    rr = (ResourceRecord*)(res);
                    res = res + sizeof(ResourceRecord);

                    if (res + sizeof(ResourceRecord) - (unsigned char*)RR > response)
                    {
                        printf("++\tinvalid record: truncated RR answer header\n");
                        errorflag = true;
                        closesocket(sock);
                        WSACleanup();
                        return;
                    }

                    if (ntohs(rr->type) == DNS_A)
                    {
                        if (res + (int)ntohs(rr->data_len) - (unsigned char*)RR > response)
                        {
                            printf("\t ++ invalid record: RR value length beyond packet\n");
                            errorflag = true;
                            closesocket(sock);
                            WSACleanup();
                            return;
                        }
                        for (int j = 0; j < 4; j++)
                            printf("%d.", 16 * (unsigned char(res[j]) >> 4) + (unsigned char(res[j]) & 0x0f));
                        printf(" TTL = %d\n", ((int)ntohs(rr->ttl)));
                        res = res + 4;
                    }
                    else if ((int)ntohs(rr->type) == DNS_PTR ||  (int)ntohs(rr->type) == DNS_NS || (int)ntohs(rr->type) == DNS_CNAME)
                    {
                        switch ((int)(ntohs(rr->type)))
                        {
                            case DNS_PTR:
                                printf("PTR ");
                                break;
                            case DNS_NS:
                                printf("NS ");
                                break;
                            case DNS_CNAME:
                                printf("CNAME ");
                                break;
                        };
                        if (res + (int)ntohs(rr->data_len) - (unsigned char*)RR > response)
                        {
                            printf("\t ++ invalid record: RR value length beyond packet\n");
                            errorflag = true;
                            closesocket(sock);
                            WSACleanup();
                            return;
                        }
                        res = readRRwithJumps((unsigned char*)RR, (unsigned char*)res, reader, response);
                        printf(" TTL = %d\n", ntohs(rr->ttl));
                    }
                }
            }
            break;
        }
        else if (available == SOCKET_ERROR)
        {
            int res =0 ;
            if (res == WSAEINVAL)
            {
                end = clock();
                printf("timeout in %d ms\n", int((1000 * ((double)(end - start) / CLOCKS_PER_SEC))));
            }
            printf("failed with %d on recv\n", WSAGetLastError());
            errorflag = true;
            closesocket(sock);
            WSACleanup();
            return;
        }
    }
}

DNS_Client::~DNS_Client()
{
    WSACleanup();
    closesocket(sock);
}