#pragma once

#include "networking.hpp"
#include "internal.hpp"

namespace SUS {

class Client: public Internal::EventPoller {
public:
	Client(const std::string& _host = "127.0.0.1", const std::string& _port = SUS_DEFAULT_PORT) {
		m_Host = _host;
		m_Port = _port;
		
		m_ID = INVALID_SOCKET;
		m_Connected = false;

		m_Init = new Internal::Initialiser();

		m_TCPConnection = new Internal::Connection();
		m_UDPConnection = new Internal::Connection();

		m_TCPConnection->Create(m_Host.c_str(), m_Port.c_str(), Internal::Connection::Type::TCP);
		m_UDPConnection->Create(m_Host.c_str(), m_Port.c_str(), Internal::Connection::Type::UDP);

		SUS_DEB("Socket opened\n");
	}
	~Client() {
		delete m_TCPConnection;
		delete m_UDPConnection;
		delete m_Init;
	}

	bool Connect() {
		if(connect(m_TCPConnection->GetSocket(), m_TCPConnection->GetAddrInfo()->ai_addr, (int)m_TCPConnection->GetAddrInfo()->ai_addrlen) == SOCKET_ERROR) {
			SUS_DEB("Couldn't connect to the socket!\n");
			return false;
		}
		else {
			SUS_DEB("Socket connected\n");
			m_TCPConnection->MakeNonBlocking();
			m_UDPConnection->MakeNonBlocking();
			m_Connected = true;
			return true;
		}
	}
	bool IsConnected() const {
		return m_Connected;
	}

	SOCKET GetId() const {
		return m_ID;
	}

	void Disconnect() {
		m_Connected = false;
	}

	void Update() {
		SUS_DEB("Update:\n");
		UpdateEvents();
		UpdateData();
	}

	DEFINE_SEND_METHODS();
	void Send(const Internal::Data& _data, Protocol _prot = Protocol::TCP) {
		Internal::DataSerialiser serialised(_data);
		if(_prot == Protocol::TCP) {
			send(m_TCPConnection->GetSocket(), (const char*)serialised.GetData(), 4 + _data.Size, 0);
		}
		else {
			sendto(
				m_UDPConnection->GetSocket(), 
				(const char*)serialised.GetData(), 4 + _data.Size,
				0, m_UDPConnection->GetAddrInfo()->ai_addr, 
				(int)m_UDPConnection->GetAddrInfo()->ai_addrlen
			); 
		}
	}

protected:
	void UpdateData() {
		const uint32_t BUFFER_LEN = 512;
		char recvbuf[BUFFER_LEN] = {};

		// UDP Parsing
		while(true) {
			sockaddr_in udpInfo = {};
			int udpInfoLen = sizeof(udpInfo);
			int udpBytesReceived = recvfrom(m_UDPConnection->GetSocket(), recvbuf, BUFFER_LEN, 0, (sockaddr*)&udpInfo, &udpInfoLen);

			if(udpBytesReceived > 0) {
				m_UDPParser.SupplyData(recvbuf, udpBytesReceived);
			}
			else {
				break;
			}
		}

		// UDP Messages
		while(!m_UDPParser.GetDataQueue().empty()) {
			Internal::Data parsed = m_UDPParser.GetDataQueue().front();
			
			SUS_DEB("Received UDP Data, [(size:) %d, (first 4 bytes:) %d]\n", (int)parsed.Size, (int)*(uint32_t*)(parsed.Data));
			
			Event event;
			event.Type = EventType::MessageReceived;
			event.Message.ClientId = 0;
			event.Message.Protocol = Protocol::UDP;
			event.Message.Size = parsed.Size;
			event.Message.Data = (uint8_t*)parsed.Data;
			m_Events.push(event);

			m_UDPParser.GetDataQueue().pop();
		}

		// TCP Parsing
		while(true) {
			int countOfBytesReceived = recv(m_TCPConnection->GetSocket(), recvbuf, BUFFER_LEN, 0);
			bool errorEncountered = WSAGetLastError() == WSAECONNRESET;

			if(countOfBytesReceived > 0) {
				m_TCPParser.SupplyData(recvbuf, countOfBytesReceived);
			}
			else if(countOfBytesReceived == 0 || errorEncountered) {
				SUS_DEB("Socket disconnected!\n");
				
				delete m_TCPConnection;
				delete m_UDPConnection;

				m_TCPConnection = new Internal::Connection();
				m_UDPConnection = new Internal::Connection();

				m_TCPConnection->Create(m_Host.c_str(), m_Port.c_str(), Internal::Connection::Type::TCP);
				m_UDPConnection->Create(m_Host.c_str(), m_Port.c_str(), Internal::Connection::Type::UDP);
				
				m_ID = INVALID_SOCKET;
				m_Connected = false;

				break;
			}
			else {
				break;
			}
		}

		// TCP Messages
		while(!m_TCPParser.GetDataQueue().empty()) {
			Internal::Data parsed = m_TCPParser.GetDataQueue().front();
			m_TCPParser.GetDataQueue().pop();
			
			if(m_ID == INVALID_SOCKET && parsed.Size == sizeof(SOCKET)) {
				const uint32_t FIRST_MSG_SIZE = 4+sizeof(SOCKET);

				m_ID = *(SOCKET*)(parsed.Data);
				free(parsed.Data);

				char msg[FIRST_MSG_SIZE] = {};
				*(uint32_t*)(msg) = sizeof(SOCKET);
				*(SOCKET*)(msg+sizeof(uint32_t)) = m_ID;

				sendto( // Sends the SOCKET Id over UDP
					m_UDPConnection->GetSocket(), 
					(const char*)msg, FIRST_MSG_SIZE,
					0, m_UDPConnection->GetAddrInfo()->ai_addr, 
					(int)m_UDPConnection->GetAddrInfo()->ai_addrlen
				);

				SUS_DEB("Ping-Ponged Socket UDP Data, [%d:'%d']\n", (int)FIRST_MSG_SIZE, (int)m_ID);
			}
			else {
				SUS_DEB("Received TCP Data, [(size:) %d, (first 4 bytes:) %d]\n", (int)parsed.Size, (int)*(uint32_t*)(parsed.Data));

				Event event;
				event.Type = EventType::MessageReceived;
				event.Message.ClientId = 0;
				event.Message.Protocol = Protocol::TCP;
				event.Message.Size = parsed.Size;
				event.Message.Data = (uint8_t*)parsed.Data;
				m_Events.push(event);
			}
		}
	}

protected:
	std::string m_Host;
	std::string m_Port;

	bool m_Connected;

	SOCKET m_ID;

	Internal::DataParser m_TCPParser;
	Internal::DataParser m_UDPParser;

	Internal::Connection* m_TCPConnection;
	Internal::Connection* m_UDPConnection;
	Internal::Initialiser* m_Init;
};

}