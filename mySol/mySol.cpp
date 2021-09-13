/* Author: Abhishek Sinha
*  Class : CS612
* Semester: Fall 2021
*/

#include "pch.h"
#include "Utilities.h"

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        printf("Wrong number of arguments passed. Enter only file path and number of threads\n");
        return 0;
    }

    Socket sk;
    char *chr1 = argv[1];

    int threadCount = atoi(argv[2]);
    if (threadCount != 1)
    {
        printf("Thread count passed should be one\n");
        return 0;
    }

    //Get HTML links from the file
    //char URL_temp[MAX_URL_LEN];
    //char *fileBuf = parseFile(argv[1]);
    //while (fileBuf != NULL)
    //{
    //    printf("Hello there123\n");
    //    sprintf(URL_temp, "%s", fileBuf);
    //    int ret_status = 9;
    //    printf("%d %d\n",ret_status, strlen(URL_temp));
    //    /*if (ret_status != strlen(URL_temp))
    //    {
    //        printf("\t  Unable to read URL from buffer\n");
    //        return 0;
    //    }
    //    else
    //    {
    //        printf("Hello there\n");
    //        printf("%s %d\n", URL_temp, strlen(URL_temp));
    //    }*/
    //    fileBuf += strlen(URL_temp) + 1;
    //}

    parseFile_gets(argv[1]);
    //Move to next phase only if previous successful
    bool next_mv = false;

    //URL parsing phase
    next_mv = sk.Get(chr1);

    //Socket connection phase
    if(next_mv)
        next_mv = sk.init_sock(sk.hostName);

    //HTML file parsing phase
    if (next_mv)
    {
        if (sk.buff[9] == '2')
            sk.HTMLfileParser(chr1);
        else
            sk.Display_Stats();
    }

    return 0;
}
