#include "networking.hpp"
#include <time.h>

template<typename T> T randomRange(T min, T max) {
	return min + ((T)std::rand() % ( min - max + (T)1 ));
}

#define POS 32
struct Pair {
	int a = 0;
	int b = 0;
};

int main() {
	printf("This is the Client:\n");

	Net::Internal::Client client;

	while(!client.Connect()) {
		std::this_thread::sleep_for(1000ms);
	}

	srand(time(NULL));
	Pair xy = {randomRange(-50, 50), randomRange(-50, 50)};

	while(client.IsConnected()) {
		std::this_thread::sleep_for(1000ms);
		client.Update();

		if(auto opt = client.Get(POS)) {
			Pair p = *(Pair*)((*opt).Data);

			std::cout << "Received: X: " << p.a << " Y: " << p.b << std::endl;
		}

		client.Send(POS, xy);
		std::cout << "Sent: X: " << xy.a << " Y: " << xy.b << std::endl;
	}

	printf("Client Closed!\n");
	std::cin.get();
}