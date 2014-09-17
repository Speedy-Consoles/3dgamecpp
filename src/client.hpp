#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <chrono>
#include "server_interface.hpp"
#include "world.hpp"
#include "stopwatch.hpp"
#include "menu.hpp"

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
	CLOCK_FSH,
	CLOCK_ALL,

	CLOCK_ID_NUM
};

class Client {
private:
	ServerInterface *serverInterface = nullptr;
	World *world = nullptr;
	Menu *menu = nullptr;
	Graphics *graphics = nullptr;
	GraphicsConf *conf = nullptr;
	Stopwatch *stopwatch = nullptr;

	int localClientID;

	std::chrono::time_point<std::chrono::high_resolution_clock> startTimePoint;

	int64 time = 0;
	int64 timeShift = 0;

	bool closeRequested = false;

public:
	Client(const Client &) = delete;
	Client();
	~Client();

	void run();

private:
	void sync(int perSecond);

	void handleInput();
};

#endif // CLIENT_HPP
