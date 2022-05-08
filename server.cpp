#include "networking.hpp"

int main() {
	printf("This is the Server:\n");

	// Init
	WSAData wsaData;
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ERROR_OUT("Couldn't start up WSA!\n");
	}

	

	std::cin.get();
}