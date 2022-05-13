#pragma once

#include "networking.hpp"

namespace SUS {
namespace Internal {

struct Data {
	uint32_t Size = 0;
	void* Data = nullptr;
};

class DataParser {
public:
	DataParser() {
		m_Parsing = 0;
		m_SizeNeeded = 0;
	}
	~DataParser() {
		
	}

	bool ParseBytes() {
		if(!m_Parsing && m_BytesReceived.size() >= 4) {
			m_SizeNeeded = 4 + (*(uint32_t*)&m_BytesReceived[0]);
			m_Parsing = true;
		}
		if(m_Parsing) {
			uint32_t copySize = min(m_BytesReceived.size(), m_SizeNeeded);
			size_t beforeResize = m_BytesParsed.size();
			m_BytesParsed.resize(beforeResize + copySize);
			memcpy(&m_BytesParsed[beforeResize], &m_BytesReceived[0], copySize);
			m_BytesReceived.erase(m_BytesReceived.begin(), m_BytesReceived.begin() + copySize);
			
			m_SizeNeeded -= copySize;
			if(m_SizeNeeded <= 0) {
				m_Parsing = false;
				return true;
			}
		}
		return false;
	}

	void Receive(uint32_t _size, void* _data) {
		size_t beforeResize = m_BytesParsed.size();
		m_BytesReceived.resize(beforeResize + 4 + _size);
		memcpy(&m_BytesReceived[beforeResize], _data, _size);
	}

	const std::vector<uint8_t>& GetParsed() {
		return m_BytesParsed;
	}

protected:
	uint32_t m_SizeNeeded;
	bool m_Parsing;
	std::vector<uint8_t> m_BytesReceived;
	std::vector<uint8_t> m_BytesParsed;
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

}
}
