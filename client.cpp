#include "networking.hpp"

int main() {
	printf("This is the Client:\n");

	Net::Internal::Client client;

	while(!client.Connect()) {
		std::this_thread::sleep_for(1000ms);
	}

	printf("Client Closed!\n");
	std::cin.get();
}