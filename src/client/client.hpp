#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "engine/std_types.hpp"
#include "engine/time.hpp"

class ServerInterface;
class World;
class Menu;
class Graphics;
struct GraphicsConf;
class Stopwatch;

class Client {
public:
	enum class State {
		CONNECTING,
		PLAYING,
		IN_MENU,
	};

	Client(const char *worldId, const char *serverAdress);
	~Client();

	void run();

private:
	ServerInterface *serverInterface = nullptr;
	World *world = nullptr;
	Menu *menu = nullptr;
	Graphics *graphics = nullptr;
	GraphicsConf *conf = nullptr;
	Stopwatch *stopwatch = nullptr;

	uint8 localClientId;

	State state = State::CONNECTING;

	Time time = 0;
    Time timeShift = 0;

	bool closeRequested = false;


public:


private:
	void sync(int perSecond);

	void handleInput();
};

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

#endif // CLIENT_HPP
