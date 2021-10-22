// DNS_client_n_query.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "DNS_query.h"
#include "DNS_Client.h"

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        printf("Invalid number of parameters\n");
        return 0;
    }

    /*char str[] = "www.dhs.gov";
    char dns[] = "128.194.135.79";
    char dns[] = "127.0.0.1";
    char host2[] = "192.168.1.34";*/
    DNS_Client dc;
    int Qtype = 0;
    char* revIP;
    if (dc.query_type_identifier(argv[1]) == DNS_A)
    {
        Qtype = DNS_A;
        printf("%-12s%s%s\n", "Lookup", ": ", argv[1]);
        dc.query_generator(Qtype, argv[1]);
        dc.response_parser(argv[2]);
    }
    else
    {
        revIP = reverseIP(argv[1]);
        Qtype = DNS_PTR;
        printf("%-12s%s%s\n", "Lookup", ": ", revIP);
        dc.query_generator(Qtype, revIP);
        dc.response_parser(argv[2]);
        free(revIP);
    }
    
    return 0;
}
