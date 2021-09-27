/* Author: Abhishek Sinha
*  Class : CS612
* Semester: Fall 2021
*/

#include "pch.h"
#include "Utilities.h"

extern struct msg;
extern CRITICAL_SECTION lpCriticalSection;
extern clock_t start_time;
//extern concurrency::concurrent_queue<msg> Q;
extern queue<msg> Q;

int main(int argc, char** argv)
{
    Crawler cr;
    int numTh = atoi(argv[1]) + 2;
    HANDLE* handles = new HANDLE[numTh + 2];
    InitializeCriticalSection(&lpCriticalSection);
    cr.Producer(argv[2]);
    InterlockedAdd(&cr.numberThreads, numTh);
    cr.hEvent = CreateEventA(NULL, true, false, NULL);
    for (int i = 0; i < numTh; i++)
    {
        handles[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Consumer, &cr, 0, NULL);
    }
    handles[numTh] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)stats, &cr, 0, NULL);
    handles[numTh+1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)final_stat, &cr, 0, NULL);


    for (int i = 0; i < numTh+2; i++)
    {
        WaitForSingleObject(handles[i], INFINITE);
        CloseHandle(handles[i]);
    }
   
    return 0;
}