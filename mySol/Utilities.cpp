#include "Utilities.h"
#include "pch.h"

struct msg {
    char URLName[MAX_URL_LEN];
};

//concurrency::concurrent_queue<msg> Q;
queue<msg> Q;
CRITICAL_SECTION lpCriticalSection;

bool Crawler::Producer(char *fileName)
{
    FILE* fptr;
    fptr = fopen(fileName, "rb");
    
    char URL_from_file[MAX_URL_LEN];
    if (fptr == NULL)
    {
        printf("Unable to open file. Please ensure that file path is correct\n");
        return false;
    }
    else
    {
        while (fscanf(fptr, "%[^\n] ", &URL_from_file) != EOF)
        {
            msg P; 
            memcpy(&P.URLName, URL_from_file, strlen(URL_from_file)+1);
            Q.push(P);
        }
    }

    fclose(fptr);
    return true;
}

void Consumer(LPVOID pParam)
{
    Crawler* cr = ((Crawler*)pParam);
    Socket sk;
    msg P;
    HTMLParserBase* parser = new HTMLParserBase;
    while (1)
    {
        EnterCriticalSection(&lpCriticalSection);
        if (Q.size() == 0)
            break;
        P = Q.front();
        if (P.URLName == NULL)
            continue;
        Q.pop();
       /* if (Q.try_pop(P))
        {*/
            LeaveCriticalSection(&lpCriticalSection);
            //first get the robots, which means flag==3
            bool next_mv = sk.Get(P.URLName, 3, true);
            if (next_mv)
            {
                InterlockedAdd(&cr->QueueUsed, 1);
                //cr->QueueUsed++;
                int prevSize = sk.seenHosts.size();
                sk.seenHosts.insert(sk.hostName);
                if (sk.seenHosts.size() > prevSize && next_mv)
                {
                    InterlockedAdd(&cr->Hostunique, 1);
                    //cr->Hostunique++;
                    next_mv = sk.init_sock(sk.hostName, 2, pParam);
                    if (next_mv)
                        next_mv = sk.Get(P.URLName, 2, false);

                    int prev_pos = sk.curPos;
                    if (next_mv)
                    {
                        next_mv = sk.init_sock(sk.hostName, 1, pParam);
                        InterlockedAdd(&cr->dataBytes, sk.curPos);
                       // cr->dataBytes++;
                    }
                    if (next_mv)
                    {
                        if (sk.buff[9] == '2')
                        {
                            // where this page came from; needed for construction of relative links
                            int numLinks;
                            char* linkBuffer = parser->Parse(&sk.buff[prev_pos], sk.curPos - prev_pos, P.URLName, (int)strlen(P.URLName), &numLinks);

                            // check for errors indicated by negative values
                            if (numLinks < 0)
                                numLinks = 0;
                            InterlockedAdd(&cr->nLinks, numLinks);
                            //cr->nLinks++;
                        }
                    }
                }
            }
        //}
    }
    InterlockedAdd(&cr->numberThreads, -1);
    delete parser;
}

void stats(LPVOID pParam)
{
    Crawler* cr = ((Crawler*)pParam);
    clock_t start_time = clock();
    clock_t time_req;
    float time_elapsed = 0;
    while (1)
    {
        Sleep(2000);
        time_req = clock();
        EnterCriticalSection(&lpCriticalSection);
        if (Q.size() == 0)
            break;
        time_elapsed = (time_req - start_time) / CLOCKS_PER_SEC;
        printf("[%3d] %d  Q:%6d  E:%6d  H:%6d  D:%6d  I:%5d  R:%5d  C:%5d  L:%4dK\n", (int)time_elapsed, cr->numberThreads, Q.size(), cr->QueueUsed, cr->Hostunique, cr->DNSLookups, cr->IPUnique, cr->robot, (cr->http_check2+ cr->http_check3+ cr->http_check4+ cr->http_check5+ cr->other), cr->nLinks/1000);
        float pps = (float)(cr->Hostunique)/ time_elapsed;
        printf("*** crawling %3.1f pps @ %3.1f Mbps\n", pps, (float)cr->dataBytes*8 /(1000000* time_elapsed));
        LeaveCriticalSection(&lpCriticalSection);
    }
    if (Q.empty())
    {
        time_req = clock();
        time_elapsed = (time_req - start_time) / CLOCKS_PER_SEC;
        printf("\nExtracted %d URLs @ %d/s\nLooked up %d DNS names @ %6/s\n", cr->QueueUsed, (int)(cr->QueueUsed / time_elapsed), cr->DNSLookups, (int)(cr->DNSLookups / time_elapsed));
        printf("Attempted %d URLs @ %d/s\nCrawled %d pages @ %d/s(%f MB)\n", cr->robot, (int)(cr->robot / time_elapsed), cr->Hostunique, (int)(cr->Hostunique / time_elapsed),cr->dataBytes/(1024*1024));
        printf("Parsed %d links @ %d/s\nHTTP codes : 2xx = %d 3xx = %d 4xx = %d 5xx = %d  other = %d\n", cr->nLinks, (int)(cr->nLinks / time_elapsed), cr->http_check2, cr->http_check3, cr->http_check4, cr->http_check5, cr->other);
            //HTTP codes : 2xx = 47185, 3xx = 5826, 4xx = 6691, 5xx = 202, other = 0
    }
}