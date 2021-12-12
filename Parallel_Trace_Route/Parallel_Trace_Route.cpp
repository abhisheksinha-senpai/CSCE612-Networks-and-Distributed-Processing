// Parallel_Trace_Route.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "pch.h"
#include "ParallelTrace.h"

using namespace std;

int main(int argc, char** argv)
{
    try {
        ParallelTrace trace;
        trace.websites.push(argv[1]);
        trace.Traceroute(trace.GetNextIP());
    }
    catch (int statusCode)
    {
        printf("Trace route failed with error code %d\n", statusCode);
    }
    return 0;
}
