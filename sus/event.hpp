#pragma once

namespace SUS {

namespace _Event {
	struct Client {
		SOCKET Id;
	};
	struct Message {
		SOCKET ClientId;
		Protocol Protocol;
		uint32_t Size;
		void* Data;
	};
}

enum class EventType {
	ClientConnected,
	ClientDisconnected,

	MessageReceived
};

struct Event {
	EventType Type;

	union {
		_Event::Client Client;
		_Event::Message Message;
	};
};

}