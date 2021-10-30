// UDP_Transport_layer_service.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

int main()
{
    // parse command-line parameters 
    char* targetHost = ...
        int power = atoi(...); // command-line specified integer 
    int senderWindow = atoi(...); // command-line specified integer 
    UINT64 dwordBufSize = (UINT64)1 << power;
    DWORD* dwordBuf = new DWORD[dwordBufSize]; // user-requested buffer 
    for (UINT64 i = 0; i < dwordBufSize; i++) // required initialization 
        dwordBuf[i] = i;

    SenderSocket ss; // instance of your class 
    LinkProperties lp;
    lp.RTT = atof(...);
    lp.speed = 1e6 * atof(...); // convert to megabits 
    lp.pLoss[FORWARD_PATH] = atof(...);
    lp.pLoss[RETURN_PATH] = atof(...);
    if ((status = ss.Open(targetHost, MAGIC_PORT, senderWindow, &lp)) != STATUS_OK)
        // error handling: print status and quit 
            
    char* charBuf = (char*)dwordBuf; // this buffer goes into socket 
    UINT64 byteBufferSize = dwordBufSize << 2; // convert to bytes 
    UINT64 off = 0; // current position in buffer 
    while (off < byteBufferSize)
    {
        // decide the size of next chunk 
        int bytes = min(byteBufferSize - off, MAX_PKT_SIZE - sizeof(SenderDataHeader));
        // send chunk into socket 
        if ((status = ss.Send(charBuf + off, bytes)) != STATUS_OK)
            // error handing: print status and quit 
            off += bytes;
    }
    if ((status = ss.Close()) != STATUS_OK)
        // error handing: print status and quit
}
