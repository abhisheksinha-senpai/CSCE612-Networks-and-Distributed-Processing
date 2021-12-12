// Parallel_Trace_Route.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "pch.h"
#include "ParallelTrace.h"
#include "Utilities.h"
using namespace std;

#ifdef BACTH_MODE

extern CRITICAL_SECTION lpCriticalSection;
int success = 0;
void TraceThread(LPVOID pParam)
{
    ParallelTrace trace;
    do {
        try {
            trace.Traceroute(trace.GetNextIP());
        }
        catch (int statusCode)
        {
            printf("Trace route failed with error code %d\n", statusCode);
        }
        EnterCriticalSection(&lpCriticalSection);
        success++;
        LeaveCriticalSection(&lpCriticalSection);
    } while (success < 5);
}
#endif

extern std::queue<std::string> websites;

int main(int argc, char** argv)
{
#ifdef BACTH_MODE

    FILE* fptr;
    fptr = fopen(argv[1], "r");

    char URL_from_file[MAX_URL_LENGTH];
    if (fptr == NULL)
    {
        printf("Unable to open file. Please ensure that file path is correct\n");
        exit(EXIT_FAILURE);
}
    else
    {
        while (fscanf(fptr, "%[^\n] ", &URL_from_file) != EOF)
        {
            string temp;
            temp.append(URL_from_file);
            websites.push(temp);
        }
    }

    fclose(fptr);

    InitializeCriticalSection(&lpCriticalSection);
    HANDLE TraceThreadHandle[3];
    for (int i = 0; i < 3; i++)
    {
        TraceThreadHandle[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TraceThread, NULL, 0, NULL);
    }

#else
    try {
        ParallelTrace trace;
        websites.push(argv[1]);
        trace.Traceroute(trace.GetNextIP());
    }
    catch (int statusCode)
    {
        printf("Trace route failed with error code %d\n", statusCode);
    }
#endif

    return 0;
}


