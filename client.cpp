#include "networking.hpp"

int main() {
	printf("This is the Client:\n");

	// Init
	WSAData wsaData;
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ERROR_OUT("Couldn't start up WSA!\n");
	}

	// Get address info (TCP)
	addrinfo hints = {};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Specify both the port and the address, cuz we're the one connecting
	addrinfo* result = nullptr;
	if(getaddrinfo("127.0.0.1", SERVER_PORT, &hints, &result) != 0) {
		ERROR_OUT("Couldn't get address info!");
	}

	SOCKET connectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if(connectSocket == INVALID_SOCKET) {
		ERROR_OUT("Couldn't open socket!");
	}
	printf("Socket opened\n");

	if(connect(connectSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		ERROR_OUT("Couldn't connect to the socket!");
	}
	printf("Socket connected\n");

	
	// recieve some data
	const uint32_t BUFFER_LEN = 512;
	char recvbuf[BUFFER_LEN] = {};

	int a = recv(connectSocket, recvbuf, BUFFER_LEN, 0);
	if(a > 0) {
		printf("RECV %d bytes : '%s'\n", a, recvbuf);
	}
	else {
		ERROR_OUT("Couldn't recieve data!");
	}
	
	std::string datatosend = "no siemson";
	printf("SEND %d bytes : '%s'\n", (int)datatosend.size(), datatosend.c_str());

	if(send(connectSocket, &datatosend[0], datatosend.size(), 0) == SOCKET_ERROR) {
		ERROR_OUT("Couldn't send data!");
	}

	std::cin.get();
}