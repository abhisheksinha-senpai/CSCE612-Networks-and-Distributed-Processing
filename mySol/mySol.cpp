/* Author: Abhishek Sinha
*  Class : CS612
* Semester: Fall 2021
*/

#include "pch.h"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2)
        return -1;
    /*std::cout << "Hello World!\n";*/
    Socket sk;
    char *chr1 = argv[1];
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
