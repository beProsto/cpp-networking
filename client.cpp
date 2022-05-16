#include "sus/networking.hpp"
#include <chrono>
#include <thread>
using namespace std::chrono_literals;
#include <time.h>

template<typename T> T randomRange(T min, T max) {
	return min + ((T)std::rand() % ( min - max + (T)1 ));
}

struct Pair {
	int a = 0;
	int b = 0;
};

int main() {
	printf("This is the Client:\n");

	try {
		SUS::Client client;
		while(!client.Connect()) {
			std::this_thread::sleep_for(500ms);
		}

		srand(time(NULL));
		Pair xy = {randomRange(-50, 50), randomRange(-50, 50)};

		while(client.IsConnected()) {
			std::this_thread::sleep_for(500ms);
			client.Update();

			SUS::Event event;
			while(client.PollEvent(event)) {
				// The client doesn't receive any other events apart from messages,
				// so we can just assume it's a message event.
				Pair a = Pair{0,0};
				printf("Received a");
				if(event.Message.Protocol == SUS::Protocol::TCP) {
					printf(" TCP");
					// server sends a struct using the tcp channel
					a = *(Pair*)event.Message.Data;
				}
				else {
					printf("n UDP");
					// server sends a single int using the udp channel
					a.a = *(int*)event.Message.Data;
				}
				printf(" message containing %d bytes from client %d.\n", 
				event.Message.Size, (int)event.Message.ClientId);

				printf("The massage reads: [%d, %d]\n", a.a, a.b);
			}
			
			client.Send(xy);
			std::cout << "Sent: X: " << xy.a << " Y: " << xy.b << std::endl;

			client.Send(69, SUS::Protocol::UDP);
		}
	}
	catch (std::exception &e) {
		std::cout<<"Caught exception: "<<e.what()<<"\n";
	}
	
	printf("Client Closed!\n");
	std::cin.get();
}