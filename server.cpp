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
		Position pos;
		pos.x = 2;
		pos.y = 5;

		while(true) {
			std::this_thread::sleep_for(1000ms);
			server.Update();

			auto opt = server.Get(); 
			if(opt.Data) {
				Position p = *(Position*)(opt.Data);
				std::cout << "Received: X: " << p.x << " Y: " << p.y << std::endl;
			}
			pos.x += 1;
			pos.y += 2;

			server.Send(pos);
			std::cout << "Sent: X: " << pos.x << " Y: " << pos.y << std::endl;

			server.Send(pos, SUS::Protocol::UDP);
		}
	}
	catch (std::exception &e) {
		std::cout<<"Caught exception: "<<e.what()<<"\n";
	}

	printf("Server Closed!\n");
	std::cin.get();
}