/* Author: Abhishek Sinha
*  Class : CS612
* Semester: Fall 2021
*/

#include "pch.h"
#include "Utilities.h"

extern struct msg;
extern LPCRITICAL_SECTION lpCriticalSection;
extern concurrency::concurrent_queue<msg> Q;

int main(int argc, char** argv)
{
    //if (argc != 3)
    //{
    //    printf("Wrong number of arguments passed. Enter only file path and number of threads\n");
    //    return 0;
    //}

    //char* chr1 = argv[1];

    //int threadCount = atoi(argv[1]);
    ///*if (threadCount != 1)
    //{
    //    printf("Thread count passed should be one\n");
    //    return 0;
    //}*/

    //FILE* f;

    //f = fopen(argv[2], "r");

    //if (f == NULL)
    //{

    //    printf("Error with the file opening %d \n", GetLastError());
    //    return 0;
    //}
    //fseek(f, 0, SEEK_END);
    //printf("Opened %s with size %ld bytes\n", argv[2], ftell(f));
    //FILE* fptr;
    //fptr = fopen(argv[2], "rb");
    //char URL_from_file[MAX_URL_LEN];
    //bool next_mv;
    //HTMLParserBase* parser = new HTMLParserBase;
    //if (fptr == NULL)
    //{
    //    printf("Unable to open file. Please ensure that file path is correct\n");
    //    return NULL;
    //}
    //else
    //{
    //    while (fscanf(fptr, "%[^\n] ", &URL_from_file) != EOF)
    //    {
    //        //Move to next phase only if previous successful
    //        //Parse the host
    //        Socket sk;
    //        next_mv = sk.Get(URL_from_file, 3, true);
    //        if (next_mv)
    //        {
    //            int prevSize = sk.seenHosts.size();
    //            sk.seenHosts.insert(sk.hostName);
    //            if (sk.seenHosts.size() > prevSize && next_mv)
    //            {
    //                printf("\t  Checking host uniqueness... passed\n");
    //                next_mv = sk.init_sock(sk.hostName, 2);
    //                if (next_mv)
    //                    next_mv = sk.Get(URL_from_file, 2, false);

    //                int prev_pos = sk.curPos;
    //                if (next_mv)
    //                    next_mv = sk.init_sock(sk.hostName, 1);

    //                //HTML file parsing phase
    //                if (next_mv)
    //                {
    //                    if (sk.buff[9] == '2')
    //                    {
    //                        clock_t time_req;
    //                        time_req = clock();
    //                        HTMLParserBase* parser = new HTMLParserBase;

    //                        // where this page came from; needed for construction of relative links

    //                        int nLinks;
    //                        char* linkBuffer = parser->Parse(&sk.buff[prev_pos], sk.curPos - prev_pos, URL_from_file, (int)strlen(URL_from_file), &nLinks);

    //                        // check for errors indicated by negative values
    //                        if (nLinks < 0)
    //                            nLinks = 0;
    //                        time_req = clock() - time_req;
    //                        printf("\t+ Parsing page... done in %3.1f ms with %d links\n", 1000 * (float)time_req / CLOCKS_PER_SEC, nLinks);
    //                    }
    //                }
    //                /*else
    //                    continue;*/
    //            }
    //            else
    //            {
    //                printf("\t  Checking host uniqueness... failed\n");
    //                continue;
    //            }
    //        }
    //    }
    //}

    //return 0;

    Crawler cr;
    cr.Initialize(argv[2], atoi(argv[1]));

    return 0;
}