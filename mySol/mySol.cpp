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
    int numTh = atoi(argv[1]) + 1;
    HANDLE* handles = new HANDLE[numTh + 1];
    InitializeCriticalSection(&lpCriticalSection);
    cr.Producer(argv[2]);
    InterlockedAdd(&cr.numberThreads, numTh);
    for (int i = 0; i < numTh; i++)
    {
        handles[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Consumer, &cr, 0, NULL);
    }
    handles[numTh] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)stats, &cr, 0, NULL);

    for (int i = 0; i < numTh+1; i++)
    {
        WaitForSingleObject(handles[i], INFINITE);
        CloseHandle(handles[i]);
    }

    clock_t start_time = clock();
    clock_t time_req;
    float time_elapsed = 0;

    time_req = clock();
    time_elapsed = (time_req - start_time) / CLOCKS_PER_SEC;
    printf("\nExtracted %d URLs @ %d/s\nLooked up %d DNS names @ %6/s\n", cr.QueueUsed, (int)(cr.QueueUsed / time_elapsed), cr.DNSLookups, (int)(cr.DNSLookups / time_elapsed));
    printf("Attempted %d URLs @ %d/s\nCrawled %d pages @ %d/s(%f MB)\n", cr.robot, (int)(cr.robot / time_elapsed), cr.Hostunique, (int)(cr.Hostunique / time_elapsed), cr.dataBytes / (1024 * 1024));
    printf("Parsed %d links @ %d/s\nHTTP codes : 2xx = %d 3xx = %d 4xx = %d 5xx = %d  other = %d\n", cr.nLinks, (int)(cr.nLinks / time_elapsed), cr.http_check2, cr.http_check3, cr.http_check4, cr.http_check5, cr.other);
        //HTTP codes : 2xx = 47185, 3xx = 5826, 4xx = 6691, 5xx = 202, other = 0

    return 0;
}