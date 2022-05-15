#pragma once

#include "networking.hpp"
#include "internal.hpp"
#include "event.hpp"

namespace SUS {

class Server: public Internal::EventPoller {
public:
	Server(const std::string& _port = SUS_DEFAULT_PORT) {
		m_Port = _port;

		m_TCPConnection.Create(nullptr, m_Port.c_str(), Internal::Connection::Type::TCP);
		m_UDPConnection.Create(nullptr, m_Port.c_str(), Internal::Connection::Type::UDP);
		// printf("Socket opened..\n");

		m_TCPConnection.MakeNonBlocking();
		m_UDPConnection.MakeNonBlocking();

		m_TCPConnection.MakeListen();
		m_UDPConnection.MakeListen();
	}
	~Server() {

	}
	
	void Update() {
		// printf("Update:\n");
		UpdateEvents();
		UpdateClients();
		UpdateData();
	}

	DEFINE_SEND_METHODS();
	void Send(const Internal::Data& _data, Protocol _prot = Protocol::TCP) {
		Internal::DataSerialiser serialised(_data);
		if(_prot == Protocol::TCP) {
			for(uint32_t i = 0; i < m_Clients.size(); i++) {
				send(m_Clients[i].TCP, (const char*)serialised.GetData(), 4 + _data.Size, 0);
			}
		}
		else {
			for(uint32_t i = 0; i < m_Clients.size(); i++) {
				sendto(
					m_UDPConnection.GetSocket(), 
					(const char*)serialised.GetData(), 4 + _data.Size,
					0, (sockaddr*)&m_Clients[i].UDP,
					(int)m_UDPConnection.GetAddrInfo()->ai_addrlen
				);
			}
		}
	}

protected:
	void UpdateClients() {
		SOCKET clientSocket;
		while(true) {
			clientSocket = accept(m_TCPConnection.GetSocket(), nullptr, nullptr);
			if(clientSocket == INVALID_SOCKET) {
				break;
			}

			// printf("Client connected: %d\n", (int)clientSocket);
			Internal::Socket newClient = {};
			newClient.TCP = clientSocket;
			m_Clients.push_back(newClient);

			// First TCP Message contains the client's SOCKET, it sends it back using UDP
			const size_t MSG_SIZE = sizeof(uint32_t) + sizeof(SOCKET);
			char msg[MSG_SIZE] = {};
			*(uint32_t*)(msg)  = sizeof(SOCKET);
			*(SOCKET*)(msg+sizeof(uint32_t)) = clientSocket;
			send(clientSocket, msg, MSG_SIZE, 0);

			Event event;
			event.Type = EventType::ClientConnected;
			event.Client.Id = clientSocket;
			m_Events.push(event);
		}
	}
	void UpdateData() {
		const uint32_t BUFFER_LEN = 512;
		char recvbuf[BUFFER_LEN] = {};

		// UDP Messages
		while(true) {
			sockaddr_in udpInfo = {};
			int udpInfoLen = sizeof(udpInfo);
			int udpBytesReceived = recvfrom(m_UDPConnection.GetSocket(), recvbuf, BUFFER_LEN, 0, (sockaddr*)&udpInfo, &udpInfoLen);

			if(udpBytesReceived > 0) {
				// Stringify udp client info
				std::string id(std::move(Internal::IdUDP(udpInfo)));

				if(m_UDPMap.find(id) == m_UDPMap.end()) {
					// First UDP Message contains the client's SOCKET
					uint32_t size = *(uint32_t*)recvbuf;
					uint32_t data = *(uint32_t*)(recvbuf+4);
					// printf("UDP Connection established: %d\n", data);
					for(size_t i = 0; i < m_Clients.size(); i++) {
						if(m_Clients[i].TCP == data) {
							m_Clients[i].UDP = udpInfo;
							m_UDPMap[id] = m_Clients[i].TCP;
							break;
						}
					}
				}
				else {
					uint32_t size = *(uint32_t*)recvbuf;
					uint8_t* data = (uint8_t*)(recvbuf+4);

					Event event;
					event.Type = EventType::MessageReceived;
					event.Message.ClientId = m_UDPMap[id];
					event.Message.Protocol = Protocol::UDP;
					event.Message.Size = size;
					event.Message.Data = malloc(size);
					memcpy(event.Message.Data, data, size);
					m_Events.push(event);
				}
			}
			else {
				break;
			}
		}

		// TCP Messages
		for(uint32_t i = 0; i < m_Clients.size(); i++) {
			const SOCKET sock = m_Clients[i].TCP;

			int countOfBytesReceived = recv(sock, recvbuf, BUFFER_LEN, 0);
			bool errorEncountered = WSAGetLastError() == WSAECONNRESET;

			if(countOfBytesReceived > 0) {
				// printf("Received Data from %d, [%d:'%s']\n", (int)sock, countOfBytesReceived, recvbuf);

				uint32_t size = *(uint32_t*)recvbuf;
				uint8_t* data = (uint8_t*)(recvbuf+4);

				Event event;
				event.Type = EventType::MessageReceived;
				event.Message.ClientId = sock;
				event.Message.Protocol = Protocol::TCP;
				event.Message.Size = size;
				event.Message.Data = malloc(size);
				memcpy(event.Message.Data, data, size);
				m_Events.push(event);
			}
			else if(errorEncountered) {
				closesocket(sock);
				// printf("Client disconnected: %d\n", (int)sock);
				m_Clients.erase(m_Clients.begin() + i);

				Event event;
				event.Type = EventType::ClientDisconnected;
				event.Client.Id = sock;
				m_Events.push(event);
			}
		}
	}

protected:
	std::string m_Port;

	std::vector<Internal::Socket> m_Clients;
	std::unordered_map<std::string, SOCKET> m_UDPMap;

	Internal::Connection m_TCPConnection;
	Internal::Connection m_UDPConnection;
	Internal::Initialiser m_Init;
};

}