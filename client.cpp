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
			std::this_thread::sleep_for(1000ms);
		}

		srand(time(NULL));
		Pair xy = {randomRange(-50, 50), randomRange(-50, 50)};

		while(client.IsConnected()) {
			std::this_thread::sleep_for(4000ms);
			client.Update();

			auto opt = client.Get();
			if(opt.Data) {
				Pair p = *(Pair*)(opt.Data);
				std::cout << "Received: X: " << p.a << " Y: " << p.b << std::endl;
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