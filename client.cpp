#include "networking.hpp"

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

	Pair xy = {-50, -32};

	while(client.IsConnected()) {
		std::this_thread::sleep_for(1000ms);
		client.Update();

		if(auto opt = client.Get(POS)) {
			Pair pos;
			pos = *(Pair*)((*opt).Data);

			std::cout << "X: " << pos.a << " Y: " << pos.b << std::endl;
		}

		xy.a += 2;
		xy.b += 1;

		Net::Internal::Data data;
		data.Id = POS;
		data.Size = sizeof(xy);
		data.Data = &xy;

		client.Send(data);
	}

	printf("Client Closed!\n");
	std::cin.get();
}