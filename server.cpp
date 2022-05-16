#include "sus/networking.hpp"
#include <chrono>
#include <thread>
using namespace std::chrono_literals;

struct Position {
	int x = 0;
	int y = 0;
};

int main() {
	printf("This is the Server:\n");

	try {
		SUS::Server server;

		while(true) {
			std::this_thread::sleep_for(500ms);
			server.Update();

			SUS::Event event;
			while(server.PollEvent(event)) {
				switch(event.Type) {
					case SUS::EventType::ClientConnected: {
						printf("Client %d just connected!\n", (int)event.Client.Id);
					} break;
					case SUS::EventType::ClientDisconnected: {
						printf("Client %d has disconnected!\n", (int)event.Client.Id);
						// Just testing out sending data to the clients
						server.Send((int)event.Client.Id, SUS::Protocol::UDP);
						server.Send(Position{(int)event.Client.Id, -(int)event.Client.Id}, SUS::Protocol::TCP);
					} break;
					case SUS::EventType::MessageReceived: {
						printf("Received a");
						if(event.Message.Protocol == SUS::Protocol::TCP) {
							printf(" TCP");
						}
						else {
							printf("n UDP");
						}
						printf(" message containing %d bytes from client %d.\n", 
						event.Message.Size, (int)event.Message.ClientId);
					} break;
				}
			}
		}
	}
	catch (std::exception &e) {
		std::cout<<"Caught exception: "<<e.what()<<"\n";
	}

	printf("Server Closed!\n");
	std::cin.get();
}