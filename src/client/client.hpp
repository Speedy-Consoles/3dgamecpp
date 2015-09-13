#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <memory>
#include <string>

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
class Save;
class Renderer;
struct Event;

class Client {
public:
	enum class State {
		CONNECTING,
		PLAYING,
		IN_MENU,
	};

	Client(const char *worldId, const char *serverAdress);
	~Client();

	// getter
	State getState() const { return state; }
	bool isDebugOn() const { return _isDebugOn; }
	const GraphicsConf &getConf() const { return *conf.get(); }

	// access
	Stopwatch *getStopwatch() { return stopwatch.get(); }
	Graphics *getGraphics() { return graphics.get(); }
	Menu *getMenu() { return menu.get(); }
	Save *getSave() { return save.get(); }
	BlockManager *getBlockManager() { return blockManager.get(); }
	ChunkManager *getChunkManager() { return chunkManager.get(); }
	World *getWorld() { return world.get(); }
	Renderer *getRenderer() { return renderer.get(); }
	ServerInterface *getServerInterface() { return serverInterface.get(); }

	void setConf(const GraphicsConf &);

	// convenience functions
	uint8 getLocalClientId() const;
	Player &getLocalPlayer();

	// operation
	void startLocalGame(std::string worldId);
	void startRemoteGame(std::string serverAdress);
	void exitGame();
	void run();

private:
	std::unique_ptr<Stopwatch> stopwatch;
	std::unique_ptr<GraphicsConf> conf;
	std::unique_ptr<Graphics> graphics;
	std::unique_ptr<Menu> menu;
	std::unique_ptr<Save> save; 
	std::unique_ptr<BlockManager> blockManager;
	std::unique_ptr<ChunkManager> chunkManager;
	std::unique_ptr<World> world;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<ServerInterface> serverInterface;

	State state = State::CONNECTING;

	bool _isDebugOn = false;

	Time time = 0;
    Time timeShift = 0;

	bool closeRequested = false;

private:
	void startGame();

	void sync(int perSecond);

	void handleInput();

	void handle(const Event &);
	void handleAnything(const Event &);
	void handlePlaying(const Event &);
	void handleMenu(const Event &);
};

enum ClockId {
	CLOCK_WOT, // world tick
	CLOCK_CRT, // chunk renderer tick
	CLOCK_BCH, // build chunk
	CLOCK_CRR, // chunk renderer render
	CLOCK_IBQ, // iterate render queue
	CLOCK_VS, // visibility search
	CLOCK_FSH, // glFinish
	CLOCK_FLP, // flip
	CLOCK_SYN, // synchronize
	CLOCK_ALL,

	CLOCK_ID_NUM
};

#endif // CLIENT_HPP
