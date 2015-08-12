#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <memory>

#include "shared/engine/std_types.hpp"
#include "shared/engine/time.hpp"

class ServerInterface;
class World;
class Player;
class Menu;
class Graphics;
struct GraphicsConf;
class Stopwatch;
class BlockManager;
class ChunkManager;

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
	
	const GraphicsConf &getConf() const { return *_conf.get(); }
	BlockManager *getBlockManager() { return blockManager.get(); }
	ChunkManager *getChunkManager() { return chunkManager.get(); }
	World *getWorld() { return world.get(); }
	Menu *getMenu() { return menu.get(); }
	Graphics *getGraphics() { return graphics.get(); }
	ServerInterface *getServerInterface() { return serverInterface.get(); }
	Stopwatch *getStopwatch() { return stopwatch.get(); }

	// convenience functions
	Player &getLocalPlayer();

	void setConf(const GraphicsConf &);

	void run();

private:
	std::unique_ptr<GraphicsConf> _conf;
	std::unique_ptr<BlockManager> blockManager;
	std::unique_ptr<ChunkManager> chunkManager;
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
	CLOCK_WOT, // world tick
	CLOCK_CRT, // chunk renderer tick
	CLOCK_BCH, // build chunk
	CLOCK_CRR, // chunk renderer render
	CLOCK_IRQ, // iterate render queue
	CLOCK_FSH, // glFinish
	CLOCK_FLP, // flip
	CLOCK_SYN, // synchronize
	CLOCK_ALL,

	CLOCK_ID_NUM
};

#endif // CLIENT_HPP
