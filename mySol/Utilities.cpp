#include "Utilities.h"
#include "pch.h"

struct msg {
    char URLName[MAX_URL_LEN];
};

concurrency::concurrent_queue<msg> Q;

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

void Consumer(volatile LONG* nLinks, volatile LONG* Hostunique, volatile LONG* IPUnique)
{
    Socket sk;
    msg P;
    HTMLParserBase* parser = new HTMLParserBase;
    while (!Q.empty())
    {
        if (Q.try_pop(P))
        {
            bool next_mv = sk.Get(P.URLName, 3, true);
            if (next_mv)
            {
                int prevSize = sk.seenHosts.size();
                sk.seenHosts.insert(sk.hostName);
                if (sk.seenHosts.size() > prevSize && next_mv)
                {
                    //printf("\t  Checking host uniqueness... passed\n");
                    InterlockedAdd(Hostunique, 1);
                    next_mv = sk.init_sock(sk.hostName, 2, IPUnique);
                    if (next_mv)
                        next_mv = sk.Get(P.URLName, 2, false);

                    int prev_pos = sk.curPos;
                    if (next_mv)
                        next_mv = sk.init_sock(sk.hostName, 1, IPUnique);

                    if (next_mv)
                    {
                        if (sk.buff[9] == '2')
                        {
                           // clock_t time_req;
                            //time_req = clock();

                            // where this page came from; needed for construction of relative links
                            int numLinks;
                            char* linkBuffer = parser->Parse(&sk.buff[prev_pos], sk.curPos - prev_pos, P.URLName, (int)strlen(P.URLName), &numLinks);

                            // check for errors indicated by negative values
                            if (numLinks < 0)
                                numLinks = 0;
                            InterlockedAdd(nLinks, numLinks);
                            //time_req = clock() - time_req;
                            //printf("\t+ Parsing page... done in %3.1f ms with %d links\n", 1000 * (float)time_req / CLOCKS_PER_SEC, nLinks);
                        }
                    }
                }
            }
        }
    }
}

void Crawler::stats(long start_time)
{
    EnterCriticalSection(&lpCriticalSection);
    clock_t time_req;
    time_req = clock();
    printf("[%3d] %6d  Q:%7d  E:%6d  H:%6d\n", (time_req-start_time)/CLOCKS_PER_SEC, Q.unsafe_size(), nLinks, Hostunique, IPUnique);  
    LeaveCriticalSection(&lpCriticalSection);
}

void Crawler::Initialize(char* str, int numThreads)
{
    InitializeCriticalSection(&lpCriticalSection);
    Producer(str);
    vector<thread> myThreads;
    clock_t start_time = clock();
    for (int i = 0; i < numThreads; i++)
    {
        myThreads.push_back(thread(Consumer, &nLinks, &Hostunique, &IPUnique));
    }

    while (!Q.empty())
    {
        Sleep(2000);
        stats(start_time);
    }

    for (int i = 0; i < numThreads; i++)
        myThreads[i].join();
}