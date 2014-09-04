#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <chrono>
#include "server_interface.hpp"
#include "world.hpp"
#include "stopwatch.hpp"

class Graphics;

enum ClockId {
	CLOCK_CLR,
	CLOCK_NDL,
	CLOCK_DLC,
	CLOCK_CHL,
	CLOCK_CHR,
	CLOCK_PLA,
	CLOCK_HUD,
	CLOCK_FLP,
	CLOCK_TIC,
	CLOCK_NET,
	CLOCK_SYN,
	CLOCK_ALL,

	CLOCK_ID_NUM
};

class Client {
private:
	ServerInterface *serverInterface;
	World *world;
	Graphics *graphics;

	int localClientID;

	std::chrono::time_point<std::chrono::high_resolution_clock> startTimePoint;

	int64 time;

	Stopwatch *stopwatch;

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
