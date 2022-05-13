#pragma once

/***
 * sus for networking
 * 
 * 	Simple Unreliable Solution for Networking
 * 
 * 
 * This library is a simple, unreliable solution for networking.
 * I've made this in purpose of testing out how a simple networking solution could be implemented in C++.
***/

#include <stdio.h>
#include <assert.h>
#include <iostream>

#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

#include <exception>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#define SUS_DEFAULT_PORT "2137"

namespace SUS {
	enum class Protocol {
		TCP = 0,
		UDP = 1
	};
}

#include "internal.hpp"
#include "server.hpp"
#include "client.hpp"