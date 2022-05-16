#pragma once

#include "networking.hpp"
#include "internal.hpp"
#include "event.hpp"

namespace SUS {

class Server: public Internal::EventPoller {
public:
	Server(const std::string& _port = SUS_DEFAULT_PORT) {
		m_Port = _port;

		m_Init = new Internal::Initialiser();
		m_TCPConnection = new Internal::Connection();
		m_UDPConnection = new Internal::Connection();

		m_TCPConnection->Create(nullptr, m_Port.c_str(), Internal::Connection::Type::TCP);
		m_UDPConnection->Create(nullptr, m_Port.c_str(), Internal::Connection::Type::UDP);

		m_TCPConnection->MakeNonBlocking();
		m_UDPConnection->MakeNonBlocking();

		m_TCPConnection->MakeListen();
		m_UDPConnection->MakeListen();

		SUS_DEB("Sockets opened\n");
	}
	~Server() {
		delete m_TCPConnection;
		delete m_UDPConnection;
		delete m_Init;
	}
	
	void Update() {
		SUS_DEB("Update:\n");
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
			for(auto& udp : m_UDPMap) {
				sendto(
					m_UDPConnection->GetSocket(), 
					(const char*)serialised.GetData(), 4 + _data.Size,
					0, (sockaddr*)&udp.second.UDP,
					(int)m_UDPConnection->GetAddrInfo()->ai_addrlen
				);
			}
		}
	}

protected:
	void UpdateClients() {
		SOCKET clientSocket;
		while(true) {
			clientSocket = accept(m_TCPConnection->GetSocket(), nullptr, nullptr);
			if(clientSocket == INVALID_SOCKET) {
				break;
			}

			SUS_DEB("Client connected: %d\n", (int)clientSocket);
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

		// UDP Parsing
		while(true) {
			sockaddr_in udpInfo = {};
			int udpInfoLen = sizeof(udpInfo);
			int udpBytesReceived = recvfrom(m_UDPConnection->GetSocket(), recvbuf, BUFFER_LEN, 0, (sockaddr*)&udpInfo, &udpInfoLen);

			if(udpBytesReceived > 0) {
				// Stringify udp client info
				std::string id(Internal::IdUDP(udpInfo));
				if(m_UDPMap.find(id) == m_UDPMap.end()) {
					SUS_DEB("New UDP Map Member!\n");
					m_UDPMap[id];
					m_UDPMap[id].UDP = udpInfo;
					m_UDPMap[id].TCP = INVALID_SOCKET; 
				}
				if(m_UDPParser.find(id) == m_UDPParser.end()) {
					m_UDPParser[id]; // initialise if not found
				}
				m_UDPParser[id].SupplyData(recvbuf, udpBytesReceived);
			}
			else {
				break;
			}
		}

		// UDP Messages
		for(auto& udp : m_UDPParser) {
			while(!udp.second.GetDataQueue().empty()) {
				Internal::Data parsed = udp.second.GetDataQueue().front();
				udp.second.GetDataQueue().pop();
				
				SUS_DEB("Received UDP Data, [(size:) %d, (first 4 bytes:) %d]\n", (int)parsed.Size, (int)*(uint32_t*)(parsed.Data));
				
				Event event;
				event.Type = EventType::MessageReceived;
				if(m_UDPMap[udp.first].TCP == INVALID_SOCKET) {
					if(parsed.Size == sizeof(SOCKET)) {
						SOCKET Id = *(SOCKET*)(parsed.Data);
						m_UDPMap[udp.first].TCP = Id;
						event.Message.ClientId = Id;
					}
				}
				else {
					event.Message.ClientId = m_UDPMap[udp.first].TCP;
				}
				event.Message.Protocol = Protocol::UDP;
				event.Message.Size = parsed.Size;
				event.Message.Data = (uint8_t*)parsed.Data;
				m_Events.push(event);
			}
		}
		

		// TCP Parsing
		for(uint32_t i = 0; i < m_Clients.size(); i++) {
			const SOCKET sock = m_Clients[i].TCP;
			while(true) {
				int countOfBytesReceived = recv(sock, recvbuf, BUFFER_LEN, 0);
				int err = WSAGetLastError();
				bool errorEncountered = err == WSAECONNRESET/* || err == WSAECONNABORTED || err == WSAECONNREFUSED*/;

				if(countOfBytesReceived > 0) {
					if(m_TCPParser.find(sock) == m_TCPParser.end()) {
						m_TCPParser[sock]; // initialise if not found
					}
					m_TCPParser[sock].SupplyData(recvbuf, countOfBytesReceived);
				}
				else if(countOfBytesReceived == 0 || errorEncountered) {
					SUS_DEB("Client disconnected: %d\n", (int)sock);
					
					for(auto it = m_UDPMap.cbegin(); it != m_UDPMap.cend();) {
						if (it->second.TCP == sock) {
							it = m_UDPMap.erase(it); // or m_UDPMap.erase(it++) / "it = m_UDPMap.erase(it)" since C++11
						}
						else {
							++it;
						}
					}

					closesocket(sock);
					m_Clients.erase(m_Clients.begin() + i);

					Event event;
					event.Type = EventType::ClientDisconnected;
					event.Client.Id = sock;
					m_Events.push(event);

					break;
				}
				else {
					break;
				}
			}
		}

		// TCP Messages
		for(auto& tcp : m_TCPParser) {
			while(!tcp.second.GetDataQueue().empty()) {
				Internal::Data parsed = tcp.second.GetDataQueue().front();
				tcp.second.GetDataQueue().pop();
				
				SUS_DEB("Received TCP Data, [(size:) %d, (first 4 bytes:) %d]\n", (int)parsed.Size, (int)*(uint32_t*)(parsed.Data));

				Event event;
				event.Type = EventType::MessageReceived;
				event.Message.ClientId = tcp.first;
				event.Message.Protocol = Protocol::TCP;
				event.Message.Size = parsed.Size;
				event.Message.Data = (uint8_t*)parsed.Data;
				m_Events.push(event);
			}
		}
	}

protected:
	std::string m_Port;

	std::vector<Internal::Socket> m_Clients;
	std::unordered_map<std::string, Internal::Socket> m_UDPMap;
	
	std::unordered_map<SOCKET,      Internal::DataParser> m_TCPParser;
	std::unordered_map<std::string, Internal::DataParser> m_UDPParser;

	Internal::Connection* m_TCPConnection;
	Internal::Connection* m_UDPConnection;
	Internal::Initialiser* m_Init;
};

}