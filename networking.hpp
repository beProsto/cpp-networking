#pragma once

#include <stdio.h>
#include <assert.h>
#include <iostream>

#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

#include <chrono>
#include <thread>
using namespace std::chrono_literals;


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#define DEFAULT_PORT "2137"

#define ERROR_OUT(x) { \
		printf("EXIT ERROR: %s\n", x); \
		std::cin.get(); \
		return 1; }

namespace Net {
namespace Internal {
// class Socket {
// public:
// 	Socket();
// 	~Socket();
// };
struct Data {
	SOCKET Owner = INVALID_SOCKET;
	uint32_t Id = 0;
	uint32_t Time = 0;
	uint32_t Size = 0;
	void* Data = nullptr;
};

class Server {
public:
	Server(const std::string& _port = DEFAULT_PORT) {
		m_Port = _port;

		// Init WSA
		WSAData wsaData;
		if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			throw std::runtime_error("Couldn't start up WSA!");
		}
		
		// Get address info (TCP)
		addrinfo hints = {};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Specify the port but not the address, cuz we're the one to be connected to (server)
		m_AddrInfo = nullptr;
		if(getaddrinfo(nullptr, m_Port.c_str(), &hints, &m_AddrInfo) != 0) {
			throw std::runtime_error("Couldn't get address info!");
		}

		// open the tcp socket to listen to
		m_ListenSocket = socket(m_AddrInfo->ai_family, m_AddrInfo->ai_socktype, m_AddrInfo->ai_protocol);
		if(m_ListenSocket == INVALID_SOCKET) {
			throw std::runtime_error("Couldn't open socket!");
		}
		printf("Socket opened..\n");

		// bind the socket to listen to
		if(bind(m_ListenSocket, m_AddrInfo->ai_addr, (int)m_AddrInfo->ai_addrlen) == SOCKET_ERROR) {
			throw std::runtime_error("Couldn't bind the socket!");
		}
		printf("Socket bound..\n");

		if(listen(m_ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
			throw std::runtime_error("Couldn't listen to the socket!");
		}
		printf("Socket set as listening..\n");
		
		u_long mode = 1; // set as a non-blocking socket
		ioctlsocket(m_ListenSocket, FIONBIO, &mode);
	}
	~Server() {
		closesocket(m_ListenSocket);
		freeaddrinfo(m_AddrInfo);
		WSACleanup();
	}

	void Update() {
		printf("Update:\n");
		UpdateClients();
		UpdateData();
	}

	void Send(const Data& _data) {
		for(uint32_t i = 0; i < m_Clients.size(); i++) {
			const SOCKET& sock = m_Clients[i];

			const uint32_t BUFFER_LEN = 512;
			char sndbuf[BUFFER_LEN] = {};

			uint32_t* start = (uint32_t*)sndbuf;
			*start = _data.Id;
			*(start+1) = _data.Time;
			*(start+2) = _data.Size;
			uint8_t* data = (uint8_t*)(start+3);
			memcpy(data, _data.Data, _data.Size);

			send(sock, sndbuf, sizeof(uint32_t) * 3 + _data.Size, 0);
		}
	}

	std::optional<Data> Get(uint32_t _id) {
		if(m_Data.find(_id) != m_Data.end()) {
			return m_Data[_id];
		}
		return std::nullopt;
	}

protected:
	void UpdateClients() {
		SOCKET clientSocket = accept(m_ListenSocket, nullptr, nullptr);
		if(clientSocket == INVALID_SOCKET) {
			printf("No new connetion!\n");
		}
		else {
			printf("Socket connected: %d\n", (int)clientSocket);
			m_Clients.push_back(clientSocket);
		}
	}
	void UpdateData() {
		for(uint32_t i = 0; i < m_Clients.size(); i++) {
			const SOCKET& sock = m_Clients[i];

			const uint32_t BUFFER_LEN = 512;
			char recvbuf[BUFFER_LEN] = {};

			int countOfBytesReceived = recv(sock, recvbuf, BUFFER_LEN, 0);

			if(countOfBytesReceived > 0) {
				printf("Received Data from %d, [%d:'%s']\n", (int)sock, countOfBytesReceived, recvbuf);
				
				uint32_t* start = (uint32_t*)recvbuf;
				uint32_t key = *start;
				uint32_t time = *(start+1);
				uint32_t size = *(start+2);
				uint8_t* data = (uint8_t*)(start+3);

				Data received;
				received.Owner = sock;
				received.Id = key;
				received.Time = time;
				received.Size = size;
				received.Data = malloc(size);
				memcpy(received.Data, data, size);

				if(m_Data.find(key) != m_Data.end()) {
					free(m_Data[key].Data);
				}
				m_Data[key] = received;
			}
			else if(countOfBytesReceived == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET) {
				printf("Socket disconnected: %d\n", (int)sock);
				m_Clients.erase(m_Clients.begin() + i);
				closesocket(sock);
			}
			else {
				printf("Didn't receive data from %d\n", (int)sock);
			}
		}
	}

protected:
	std::string m_Port;

	addrinfo* m_AddrInfo;
	SOCKET m_ListenSocket;
	
	std::vector<SOCKET> m_Clients;
	
	std::unordered_map<uint32_t, Data> m_Data;
};

class Client {
public:
	Client(const std::string& _host = "127.0.0.1", const std::string& _port = DEFAULT_PORT) {
		m_Host = _host;
		m_Port = _port;
		m_Connected = false;

		// Init
		WSAData wsaData;
		if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			throw std::runtime_error("Couldn't start up WSA!\n");
		}

		// Get address info (TCP)
		addrinfo hints = {};
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// Specify both the port and the address, cuz we're the one connecting
		m_AddrInfo = nullptr;
		if(getaddrinfo(m_Host.c_str(), m_Port.c_str(), &hints, &m_AddrInfo) != 0) {
			throw std::runtime_error("Couldn't get address info!");
		}

		m_ConnectSocket = socket(m_AddrInfo->ai_family, m_AddrInfo->ai_socktype, m_AddrInfo->ai_protocol);
		if(m_ConnectSocket == INVALID_SOCKET) {
			throw std::runtime_error("Couldn't open socket!");
		}
		printf("Socket opened\n");
	}
	~Client() {
		// closesocket(m_ConnectSocket); // anyone mind telling me why it breaks disconnection detection?
		freeaddrinfo(m_AddrInfo);
		WSACleanup();
	}

	bool Connect() {
		if(connect(m_ConnectSocket, m_AddrInfo->ai_addr, (int)m_AddrInfo->ai_addrlen) == SOCKET_ERROR) {
			printf("Couldn't connect to the socket!\n");
			return false;
		}
		else {
			printf("Socket connected\n");
			u_long mode = 1; // set as a non-blocking socket
			ioctlsocket(m_ConnectSocket, FIONBIO, &mode);
			m_Connected = true;
			return true;
		}
	}

	void Send(const Data& _data) {
		const uint32_t BUFFER_LEN = 512;
		char sndbuf[BUFFER_LEN] = {};

		uint32_t* start = (uint32_t*)sndbuf;
		*start = _data.Id;
		*(start+1) = _data.Time;
		*(start+2) = _data.Size;
		uint8_t* data = (uint8_t*)(start+3);
		memcpy(data, _data.Data, _data.Size);

		send(m_ConnectSocket, sndbuf, sizeof(uint32_t) * 3 + _data.Size, 0);
	}
	std::optional<Data> Get(uint32_t _id) {
		if(m_Data.find(_id) != m_Data.end()) {
			return m_Data[_id];
		}
		return std::nullopt;
	}

	bool IsConnected() const {
		return m_Connected;
	}

	void Update() {
		const uint32_t BUFFER_LEN = 512;
		char recvbuf[BUFFER_LEN] = {};

		int countOfBytesReceived = recv(m_ConnectSocket, recvbuf, BUFFER_LEN, 0);

		if(countOfBytesReceived > 0) {
			printf("Received Data from %d, [%d:'%s']\n", (int)m_ConnectSocket, countOfBytesReceived, recvbuf);

			uint32_t* start = (uint32_t*)recvbuf;
			uint32_t key = *start;
			uint32_t time = *(start+1);
			uint32_t size = *(start+2);
			uint8_t* data = (uint8_t*)(start+3);

			Data received;
			received.Owner = m_ConnectSocket;
			received.Id = key;
			received.Time = time;
			received.Size = size;
			received.Data = malloc(size);
			memcpy(received.Data, data, size);

			if(m_Data.find(key) != m_Data.end()) {
				free(m_Data[key].Data);
			}
			m_Data[key] = received;
		}
		else if(countOfBytesReceived == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET) {
			printf("Socket disconnected: %d\n", (int)m_ConnectSocket);
			m_Connected = false;
		}
		else {
			printf("Didn't receive data from %d\n", (int)m_ConnectSocket);
		}
	}

protected:
	std::string m_Host;
	std::string m_Port;
	
	addrinfo* m_AddrInfo;
	SOCKET m_ConnectSocket;
	
	bool m_Connected;

	std::unordered_map<uint32_t, Data> m_Data;
};

}
}