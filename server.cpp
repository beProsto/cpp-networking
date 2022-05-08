#include "networking.hpp"

int main() {
	printf("This is the Server:\n");

	// Init WSA
	WSAData wsaData;
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ERROR_OUT("Couldn't start up WSA!");
	}

	// Get address info (TCP)
	addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Specify the port but not the address, cuz we're the one to be connected to (server)
	addrinfo* result = nullptr;
	if(getaddrinfo(nullptr, SERVER_PORT, &hints, &result) != 0) {
		ERROR_OUT("Couldn't get address info!");
	}

	// tcp socket to listen to
	SOCKET listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if(listenSocket == INVALID_SOCKET) {
		ERROR_OUT("Couldn't open socket!");
	}
	printf("Socket opened\n");

	// bind the socket to listen to
	if(bind(listenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		ERROR_OUT("Couldn't bind the socket!");
	}
	printf("Socket bound\n");

	if(listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		ERROR_OUT("Couldn't listen to the socket!");
	}
	printf("Socket listened\n");
	

	// only happens once a client is found and connects!
	SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
	if(clientSocket == INVALID_SOCKET) {
		ERROR_OUT("Couldn't accept the client socket!");
	}
	printf("Socket accepted\n");

	// send it some data
	const uint32_t BUFFER_LEN = 512;
	char recvbuf[BUFFER_LEN] = {};

	std::string datatosend = "heja mordix";
	printf("SEND %d bytes : '%s'\n", (int)datatosend.size(), datatosend.c_str());

	if(send(clientSocket, &datatosend[0], datatosend.size(), 0) == SOCKET_ERROR) {
		ERROR_OUT("Couldn't send data!");
	}

	int a = recv(clientSocket, recvbuf, BUFFER_LEN, 0);
	if(a > 0) {
		printf("RECV %d bytes : '%s'\n", a, recvbuf);
	}
	else {
		ERROR_OUT("Couldn't recieve data!");
	}

	std::cin.get();
}