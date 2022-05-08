#pragma once

#include <stdio.h>
#include <assert.h>
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif


#define SERVER_PORT 2137

#define ERROR_OUT(x) { \
		printf("EXIT ERROR: %s\n", x); \
		std::cin.get(); \
		assert(false); }