/* Author: Abhishek Sinha
*  Class : CS612
* Semester: Fall 2021
*/

// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include "Socket.h"
#include "HTMLParserBase.h"
#include "time.h"
#include <iostream>
#include <ppl.h>
#include <vector>
#include <thread>
//#include <concurrent_queue.h>
#include <queue>
#include <winnt.h>
#include <Windows.h>
#include <synchapi.h>
#include <atomic>
#endif //PCH_H
