#include "networking.hpp"

int main() {
	printf("This is the Server:\n");

	Net::Internal::Server server;

	while(true) {
		std::this_thread::sleep_for(1000ms);
		server.Update();
	}

	printf("Server Closed!\n");
	std::cin.get();
}