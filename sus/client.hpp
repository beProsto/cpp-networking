#pragma once

#include "networking.hpp"
#include "internal.hpp"

namespace SUS {

class Client {
public:
	Client(const std::string& _host = "127.0.0.1", const std::string& _port = SUS_DEFAULT_PORT) {
		m_Host = _host;
		m_Port = _port;
		m_Connected = false;
		m_ID = -1;

		m_TCPConnection.Create(m_Host.c_str(), m_Port.c_str(), Internal::Connection::Type::TCP);
		m_UDPConnection.Create(m_Host.c_str(), m_Port.c_str(), Internal::Connection::Type::UDP);
		printf("Socket opened\n");
	}
	~Client() {

	}

	bool Connect() {
		if(connect(m_TCPConnection.GetSocket(), m_TCPConnection.GetAddrInfo()->ai_addr, (int)m_TCPConnection.GetAddrInfo()->ai_addrlen) == SOCKET_ERROR) {
			printf("Couldn't connect to the socket!\n");
			return false;
		}
		else {
			printf("Socket connected\n");
			m_TCPConnection.MakeNonBlocking();
			m_UDPConnection.MakeNonBlocking();
			
			m_Connected = true;
			return true;
		}
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
			send(m_TCPConnection.GetSocket(), (const char*)serialised.GetData(), 4 + _data.Size, 0);
		}
		else {
			sendto(
				m_UDPConnection.GetSocket(), 
				(const char*)serialised.GetData(), 4 + _data.Size,
				0, m_UDPConnection.GetAddrInfo()->ai_addr, 
				(int)m_UDPConnection.GetAddrInfo()->ai_addrlen
			); 
		}
	}

	Internal::Data Get() {
		return m_Data;
	}

	bool IsConnected() const {
		return m_Connected;
	}

	void Update() {
		const uint32_t BUFFER_LEN = 512;
		char recvbuf[BUFFER_LEN] = {};

		int countOfBytesReceived = recv(m_TCPConnection.GetSocket(), recvbuf, BUFFER_LEN, 0);

		if(countOfBytesReceived > 0) {
			printf("Received Data from %d, [%d:'%d']\n", (int)m_TCPConnection.GetSocket(), countOfBytesReceived, *(uint32_t*)recvbuf);

			uint32_t size = *(uint32_t*)recvbuf;
			uint8_t* data = (uint8_t*)(recvbuf+4);

			Internal::Data received;
			received.Size = size;
			received.Data = malloc(size);
			memcpy(received.Data, data, size);

			// First TCP Message contains the client's SOCKET, it sends it back using UDP
			if(m_ID == -1 && received.Size == 4) {
				m_ID = *(int32_t*)received.Data;
				char msg[8] = {};
				*(uint32_t*)(msg)  = 4;
				*(int32_t*)(msg+4) = m_ID;
				sendto(
					m_UDPConnection.GetSocket(), 
					msg, 8,
					0, m_UDPConnection.GetAddrInfo()->ai_addr, 
					(int)m_UDPConnection.GetAddrInfo()->ai_addrlen
				);
			}
			else {
				if(m_Data.Data) {
					free(m_Data.Data);
				}
				m_Data = received;
			}
		}
		else if(WSAGetLastError() == WSAECONNRESET) {
			printf("Socket disconnected: %d\n", (int)m_TCPConnection.GetSocket());
			m_Connected = false;
		}
		// else {
		// 	printf("Didn't receive data from %d\n", (int)m_TCPConnection.GetSocket());
		// }
		sockaddr_in info = {};
		int len = sizeof(info);
		int cnt = recvfrom(m_UDPConnection.GetSocket(), recvbuf, BUFFER_LEN, 
		0, (sockaddr*)&info, &len);
		if(cnt > 0) {
			printf("UDP Received Data from %d.%d.%d.%d:%d, [%d:'%d']\n", 
				((uint8_t*)&info.sin_addr.s_addr)[0],
				((uint8_t*)&info.sin_addr.s_addr)[1],
				((uint8_t*)&info.sin_addr.s_addr)[2],
				((uint8_t*)&info.sin_addr.s_addr)[3],
				(int)info.sin_port, 
				cnt, *(uint32_t*)recvbuf
			);
		}
}

protected:
	std::string m_Host;
	std::string m_Port;

	int32_t m_ID;
	
	bool m_Connected;

	Internal::Data m_Data;

	Internal::Connection m_TCPConnection;
	Internal::Connection m_UDPConnection;
	Internal::Initialiser m_Init;
};

}