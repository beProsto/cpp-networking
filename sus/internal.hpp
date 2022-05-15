#pragma once

#include <string>

#include "networking.hpp"
#include "event.hpp"

namespace SUS {
namespace Internal {

struct Data {
	uint32_t Size = 0;
	void* Data = nullptr;
};

class DataSerialiser {
public:
	DataSerialiser(const Internal::Data& _data) {
		m_Data = malloc(4 + _data.Size);

		uint32_t* start = (uint32_t*)m_Data;
		*start = _data.Size;
		uint8_t* data = (uint8_t*)(start+1);
		memcpy(data, _data.Data, _data.Size);
	}
	~DataSerialiser() {
		free(m_Data);
	}

	void* GetData() {
		return m_Data;
	}

protected:
	void* m_Data;
};

class Initialiser {
public:
	Initialiser() {
		WSAData wsaData;
		if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			throw std::runtime_error("Couldn't start up WSA!");
		}
	}
	~Initialiser() {
		WSACleanup();
	}
};

class Connection {
public:
	enum class Type {
		Invalid = -1,
		TCP = 0,
		UDP = 1
	};

public:
	Connection() {
		m_Type = Type::Invalid;
	}
	~Connection() {
		if(m_Type != Type::Invalid) {
			closesocket(m_Socket);
			freeaddrinfo(m_AddrInfo);
		}
	}

	void Create(const char* _host, const char* _port, Type _type = Type::TCP) {
		// Get address info (TCP)
		addrinfo hints = {};
		hints.ai_family = AF_INET;
		if(_type == Type::TCP) {
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
		}
		else {
			hints.ai_socktype = SOCK_DGRAM;
			hints.ai_protocol = IPPROTO_UDP;
		}
		hints.ai_flags = AI_PASSIVE;

		// Specify both the port and the address, cuz we're the one connecting
		m_AddrInfo = nullptr;
		if(::getaddrinfo(_host, _port, &hints, &m_AddrInfo) != 0) {
			throw std::runtime_error("Couldn't get address info!");
		}

		m_Socket = socket(m_AddrInfo->ai_family, m_AddrInfo->ai_socktype, m_AddrInfo->ai_protocol);
		if(m_Socket == INVALID_SOCKET) {
			throw std::runtime_error("Couldn't open socket!");
		}

		m_Type = _type;
	}

	void MakeNonBlocking() {
		u_long mode = 1; // set as a non-blocking socket
		ioctlsocket(m_Socket, FIONBIO, &mode);
	}

	void MakeListen() {
		// bind the socket to listen to
		if(bind(m_Socket, m_AddrInfo->ai_addr, (int)m_AddrInfo->ai_addrlen) == SOCKET_ERROR) {
			throw std::runtime_error("Couldn't bind the socket!");
		}
		printf("Socket bound..\n");

		if(m_Type == Type::TCP) {
			if(listen(m_Socket, SOMAXCONN) == SOCKET_ERROR) {
				throw std::runtime_error("Couldn't listen to the socket!");
			}
			printf("Socket set as listening..\n");
		}
	}

	addrinfo* GetAddrInfo() {
		return m_AddrInfo;
	}
	SOCKET GetSocket() {
		return m_Socket;
	}
	Type GetType() {
		return m_Type;
	}

protected:
	addrinfo* m_AddrInfo;
	SOCKET m_Socket;
	Type m_Type;
};

struct Socket {
	SOCKET TCP = INVALID_SOCKET;
	sockaddr_in UDP = {};
};

class EventPoller {
public:
	EventPoller() {}
	~EventPoller() {}

	bool PollEvent(Event& _eventRef) {
		if(!m_Events.empty()) {
			_eventRef = m_Events.front();
			m_EventsToKillNextUpdate.push(_eventRef);
			m_Events.pop();
			return true;
		}
		return false;
	}

protected:
	void UpdateEvents() {
		while(!m_EventsToKillNextUpdate.empty()) {
			Event& event = m_EventsToKillNextUpdate.front();

			if(event.Type == EventType::MessageReceived) {
				free(event.Message.Data);
			}

			m_EventsToKillNextUpdate.pop();
		}
	}

protected:
	std::queue<Event> m_Events;
	std::queue<Event> m_EventsToKillNextUpdate;
};

#define DEFINE_SEND_METHODS() \
	template <typename T> \
	void Send(const T& _data, Protocol _prot = Protocol::TCP) { \
		Internal::Data data; \
		data.Size = sizeof(T); \
		data.Data = (void*)&_data; \
		Send(data, _prot); \
	} \
	void Send(uint32_t _size, void* _data, Protocol _prot = Protocol::TCP) { \
		Internal::Data data; \
		data.Size = _size; \
		data.Data = _data; \
		Send(data, _prot); \
	}

std::string IdUDP(const sockaddr_in& _udp) {
	char id[6];
	*(uint32_t*)(id) = _udp.sin_addr.s_addr;
	*(uint16_t*)(id+4) = _udp.sin_port;
	id[0] += 1;
	id[1] += 1;
	id[2] += 1;
	id[3] += 1;
	id[4] += 1;
	id[5] += 1;
	return id;
}

}
}
