#include "networking.hpp"


#define POS 32
struct Position {
	int x = 0;
	int y = 0;
};

int main() {
	printf("This is the Server:\n");

	Net::Internal::Server server;

	Position pos;
	pos.x = 2;
	pos.y = 5;

	while(true) {
		std::this_thread::sleep_for(1000ms);
		server.Update();

		if(auto opt = server.Get(POS)) {
			Position p = *(Position*)((*opt).Data);

			std::cout << "Received: X: " << p.x << " Y: " << p.y << std::endl;
		}

		pos.x += 1;
		pos.y += 2;

		server.Send(POS, pos);
		std::cout << "Sent: X: " << pos.x << " Y: " << pos.y << std::endl;
	}

	printf("Server Closed!\n");
	std::cin.get();
}