#pragma once

#include "networking.hpp"
#include "internal.hpp"

namespace SUS {


class Server {
public:
	Server(const std::string& _port = SUS_DEFAULT_PORT) {
		m_Port = _port;

		m_TCPConnection.Create(nullptr, m_Port.c_str(), Internal::Connection::Type::TCP);
		m_UDPConnection.Create(nullptr, m_Port.c_str(), Internal::Connection::Type::UDP);
		printf("Socket opened..\n");

		m_TCPConnection.MakeNonBlocking();
		m_UDPConnection.MakeNonBlocking();

		m_TCPConnection.MakeListen();
		m_UDPConnection.MakeListen();
	}
	~Server() {

	}

	void Update() {
		printf("Update:\n");
		UpdateClients();
		UpdateData();
	}

	template <typename T>
	void Send(const T& _data, Protocol _prot = Protocol::TCP) {
		Internal::Data data;
		data.Size = sizeof(T);
		data.Data = (void*)&_data;
		Send(data, _prot);
	}
	void Send(uint32_t _size, void* _data, Protocol _prot = Protocol::TCP) {
		Internal::Data data;
		data.Size = _size;
		data.Data = _data;
		Send(data, _prot);
	}
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

	Internal::Data Get() {
		return m_Data;
	}

protected:
	void UpdateClients() {
		for(SOCKET clientSocket = accept(m_TCPConnection.GetSocket(), nullptr, nullptr); clientSocket != INVALID_SOCKET; clientSocket = accept(m_TCPConnection.GetSocket(), nullptr, nullptr)) {
			printf("Client connected: %d\n", (int)clientSocket);
			Internal::Socket newClient = {};
			newClient.TCP = clientSocket;
			m_Clients.push_back(newClient);
			// First TCP Message contains the client's SOCKET, it sends it back using UDP
			char msg[8] = {};
			*(uint32_t*)(msg)  = 4;
			*(int32_t*)(msg+4) = clientSocket;
			send(clientSocket, msg, 8, 0);
		}
	}
	void UpdateData() {
		const uint32_t BUFFER_LEN = 512;
		char recvbuf[BUFFER_LEN] = {};
		char udprecvbuf[BUFFER_LEN] = {};
		
		sockaddr_in info = {};
		int len = sizeof(info);
		int udpBytesReceived = recvfrom(m_UDPConnection.GetSocket(), udprecvbuf, BUFFER_LEN, 0, (sockaddr*)&info, &len);
		
		bool found = false;
		size_t index = 0;
		
		for(uint32_t i = 0; i < m_Clients.size(); i++) {
			const SOCKET sock = m_Clients[i].TCP;
			
			if(m_Clients[i].UDP.sin_addr.s_addr == info.sin_addr.s_addr && m_Clients[i].UDP.sin_port == info.sin_port) {
				found = true;
				index = i;
			}

			int countOfBytesReceived = recv(sock, recvbuf, BUFFER_LEN, 0);
			bool errorEncountered = WSAGetLastError() == WSAECONNRESET;

			if(countOfBytesReceived > 0) {
				printf("Received Data from %d, [%d:'%s']\n", (int)sock, countOfBytesReceived, recvbuf);

				uint32_t size = *(uint32_t*)recvbuf;
				uint8_t* data = (uint8_t*)(recvbuf+4);

				Internal::Data received;
				received.Size = size;
				received.Data = malloc(size);
				memcpy(received.Data, data, size);

				if(m_Data.Data) {
					free(m_Data.Data);
				}
				m_Data = received;
			}
			else if(errorEncountered) {
				closesocket(sock);
				printf("Client disconnected: %d\n", (int)sock);
				m_Clients.erase(m_Clients.begin() + i);
			}
		}

		if(udpBytesReceived > 0) {
			if(!found) {
				uint32_t size = *(uint32_t*)udprecvbuf;
				uint32_t data = *(uint32_t*)(udprecvbuf+4);
				printf("UDP Connection established: %d\n", data);
				for(uint32_t i = 0; i < m_Clients.size(); i++) {
					if(m_Clients[i].TCP == data) {
						m_Clients[i].UDP = info;
						printf("Set'n found!\n");
						break;
					}
				}
			}
			else {
				printf("UDP Received Data from %d.%d.%d.%d:%d, [%d:'%d']\n", 
					((uint8_t*)&info.sin_addr.s_addr)[0],
					((uint8_t*)&info.sin_addr.s_addr)[1],
					((uint8_t*)&info.sin_addr.s_addr)[2],
					((uint8_t*)&info.sin_addr.s_addr)[3],
					(int)info.sin_port,
					udpBytesReceived, *(uint32_t*)(udprecvbuf)
				);
			}

		}
	
	}

protected:
	std::string m_Port;

	std::vector<Internal::Socket> m_Clients;
	
	Internal::Data m_Data;

	Internal::Connection m_TCPConnection;
	Internal::Connection m_UDPConnection;
	Internal::Initialiser m_Init;
};

}