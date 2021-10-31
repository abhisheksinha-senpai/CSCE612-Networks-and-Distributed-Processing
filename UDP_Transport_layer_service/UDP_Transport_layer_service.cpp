// UDP_Transport_layer_service.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "UDP_Transport.h"
#include "SenderSocket.h"
#include <iostream>
#include <iomanip>

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
    printf("Main:  sender W = %d, RTT %.3f sec, loss %g / %.4f, link %f Mbps\n", senderWindow, lp.RTT, lp.pLoss[FORWARD_PATH], lp.pLoss[RETURN_PATH], lp.speed);
    
    clock_t start, end;
    start = clock();
    for (UINT64 i = 0; i < dwordBufSize; i++) // required initialization
        dwordBuf[i] = i;
    end = clock();
    printf("Main:  initializing DWORD array with 2^%d elements... done in %d ms\n", power, (int)(1000 * ((float)(end - start) / CLOCKS_PER_SEC)));
    SenderSocket ss; // instance of your class 
    if ((status = ss.Open(targetHost, MAGIC_PORT, senderWindow, lp)) != STATUS_OK)
    {
        printf("Main:  connect failed with status % d\n", status);
        exit(EXIT_FAILURE);
    }
    

    char* charBuf = (char*)dwordBuf; // this buffer goes into socket
    UINT64 byteBufferSize = dwordBufSize << 2; // convert to bytes
    UINT64 off = 0; // current position in buffer
    //while (off < byteBufferSize)
    //{
    //    // decide the size of next chunk
    //    int bytes = min(byteBufferSize - off, MAX_PKT_SIZE - sizeof(SenderDataHeader));
    //    // send chunk into socket
    //    if ((status = ss.Send (charBuf + off, bytes)) != STATUS_OK)
    //    // error handing: print status and quit
    //    off += bytes;
    //}

    if ((status = ss.Close()) != STATUS_OK)
    {
        printf("Main:  connect failed with status % d\n", status);
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
