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
			Position pos;
			pos = *(Position*)((*opt).Data);

			std::cout << "X: " << pos.x << " Y: " << pos.y << std::endl;
		}

		pos.x += 1;
		pos.y += 2;

		Net::Internal::Data data;
		data.Id = POS;
		data.Size = sizeof(pos);
		data.Data = &pos;

		server.Send(data);
	}

	printf("Server Closed!\n");
	std::cin.get();
}