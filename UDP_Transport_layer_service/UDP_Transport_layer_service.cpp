// UDP_Transport_layer_service.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "UDP_Transport.h"
#include "SenderSocket.h"
#include <iostream>
#include <iomanip>

CRITICAL_SECTION lpCriticalSection;
extern unsigned long long int last_size;

int main(int argc, char **argv)
{
    // parse command-line parameters 
    char* targetHost = argv[1];
    int power = atoi(argv[2]); // command-line specified integer 
    int senderWindow = atoi(argv[3]); // command-line specified integer
    UINT64 dwordBufSize = (UINT64)1 << power;
    DWORD* dwordBuf = new DWORD[dwordBufSize]; // user-requested buffer
    LinkProperties lp;
    lp.RTT = atof(argv[4]);
    lp.speed = 1e6 * atof(argv[7]); // convert to megabits
    lp.pLoss[FORWARD_PATH] = atof(argv[5]);
    lp.pLoss[RETURN_PATH] = atof(argv[6]);
    lp.bufferSize = senderWindow;
    int status = STATUS_OK;
    printf("Main:  sender W = %d, RTT %.3f sec, loss %g / %.4f, link %d Mbps\n", senderWindow, lp.RTT, lp.pLoss[FORWARD_PATH], lp.pLoss[RETURN_PATH], atoi(argv[7]));
    
    clock_t start, end, temp;
    start = clock();
    for (UINT64 i = 0; i < dwordBufSize; i++) // required initialization
        dwordBuf[i] = i;
    end = clock();
    printf("Main:  initializing DWORD array with 2^%d elements... done in %d ms\n", power, (int)(1000 * ((float)(end - start) / CLOCKS_PER_SEC)));

    SenderSocket ss; // instance of your class 
    temp = clock();
    if ((status = ss.Open(targetHost, MAGIC_PORT, senderWindow, lp)) != STATUS_OK)
    {
        printf("Main:  connect failed with status % d\n", status);
        exit(EXIT_FAILURE);
    }
    end = clock();
    InitializeCriticalSection(&lpCriticalSection);
    printf("Main:  connected to %s in %.3f sec, pkt size %d bytes\n", targetHost, ((float)((float)end - (float)temp) / CLOCKS_PER_SEC), MAX_PKT_SIZE);
    temp = clock();
    char* charBuf = (char*)dwordBuf; // this buffer goes into socket
    ss.userBuf = charBuf;
    UINT64 byteBufferSize = dwordBufSize << 2; // convert to bytes
    UINT64 off = 0; // current position in buffer
    ss.sendstart = clock();
    long pkt_cnt = 0;
    while (off < byteBufferSize)
    {
        pkt_cnt++;
        // decide the size of next chunk
        int bytes = min(byteBufferSize - off, MAX_PKT_SIZE - sizeof(SenderDataHeader));
        // send chunk into socket
        if ((status = ss.Send (charBuf + off, bytes)) != STATUS_OK)
        {
            printf("Main:  sendto failed with status %d\n", status);
            exit(EXIT_FAILURE);
        }
        // error handing: print status and quit
        off += bytes;
    }
    end = clock();
    if ((status = ss.Close()) != STATUS_OK)
    {
        printf("Main:  connect failed with status % d\n", status);
        exit(EXIT_FAILURE);
    }
    
    printf("Main:  transfer finished in %.3f sec, %.3f Kbps, checksum %X\n", ((float)((float)clock() - (float)temp) / CLOCKS_PER_SEC), (8 * byteBufferSize)/ (1024*(float)((float)ss.sendend - (float)temp) / CLOCKS_PER_SEC), ss.checksum);
    printf("Main: estRTT %.3f, ideal rate %.2f Kbps\n", ss.EstimatedRTT, (8* byteBufferSize)/ (1024*((float)((float)end - (float)ss.sendstart) / CLOCKS_PER_SEC)));
    exit(EXIT_SUCCESS);
}

void Stats(LPVOID pParam)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    SenderSocket* ss = ((SenderSocket*)pParam);
    long cur_time, sendtime;

    while (1)
    {
        DWORD status = WaitForSingleObject(ss->printQuit, 2000);
        if (status == WAIT_TIMEOUT)
        {
            //EnterCriticalSection(&lpCriticalSection);
            cur_time = (((double)clock() - ss->start) / CLOCKS_PER_SEC);
            sendtime = (((double)clock() - ss->sendstart) / CLOCKS_PER_SEC);
            if (cur_time % 2 == 0)
                printf("[%d] B\t %ld ( %.1f MB)  N    %d T %d F %d W %d S %.3f Mbps RTT %f\n", cur_time, ss->sendBasenumber, (float)(last_size) / (1024 * 1024), ss->nextSeqnumber, ss->retransmit_count, ss->fast_tx_count,min(ss->windowSize,ss->rcvWindsize), (float)(last_size * 8) / (sendtime * 1024 * 1024), ss->EstimatedRTT);
            //LeaveCriticalSection(&lpCriticalSection);
        }
        else
        {
            SetEvent(ss->eventQuit);
            return;
        }
    }
}
