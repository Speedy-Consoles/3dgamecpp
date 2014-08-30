#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <chrono>
#include "server_interface.hpp"
#include "world.hpp"
#include "graphics.hpp"

class Client {
private:
	ServerInterface *serverInterface;
	World *world;
	Graphics *graphics;

	int localClientID;

	std::chrono::time_point<std::chrono::high_resolution_clock> startTimePoint;

	int64 time;

	bool closeRequested = false;

public:
	Client();
	~Client();

	void run();

private:
	void sync(int perSecond);

	void handleInput();
};

#endif // CLIENT_HPP
