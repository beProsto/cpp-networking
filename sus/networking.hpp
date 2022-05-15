#pragma once

/***
 * SUS for Networking
 * 
 *  Simple Unreliable Solution for Networking
 * 
 * 
 * This library is a simple, unreliable solution for networking.
 * I've made this in purpose of testing out how a simple networking solution could be implemented in C++.
***/

#define SUS_DEFAULT_PORT "2137"

namespace SUS {
	enum class Protocol {
		TCP = 0,
		UDP = 1
	};
}

#include "pch.hpp"
#include "internal.hpp"
#include "event.hpp"
#include "server.hpp"
#include "client.hpp"