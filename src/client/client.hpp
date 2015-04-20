#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "engine/std_types.hpp"
#include "engine/time.hpp"

#include <memory>

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

	State getState() const { return state; }
	bool isPaused() const { return _isPaused; }
	bool isDebugOn() const { return _isDebugOn; }
	uint8 getLocalClientId() const;
	
	GraphicsConf *getConf() { return conf.get(); }
	World *getWorld() { return world.get(); }
	Menu *getMenu() { return menu.get(); }
	Graphics *getGraphics() { return graphics.get(); }
	ServerInterface *getServerInterface() { return serverInterface.get(); }
	Stopwatch *getStopwatch() { return stopwatch.get(); }

	void run();

private:
	std::unique_ptr<GraphicsConf> conf;
	std::unique_ptr<World> world;
	std::unique_ptr<Menu> menu;
	std::unique_ptr<Graphics> graphics;
	std::unique_ptr<ServerInterface> serverInterface;
	std::unique_ptr<Stopwatch> stopwatch;

	uint8 localClientId;

	State state = State::CONNECTING;

	bool _isPaused = false;
	bool _isDebugOn = false;

	Time time = 0;
    Time timeShift = 0;

	bool closeRequested = false;

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
