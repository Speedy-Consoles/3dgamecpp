#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <memory>
#include <string>
#include <vector>

#include "shared/engine/std_types.hpp"
#include "shared/engine/time.hpp"

class ServerInterface;
class LocalServerInterface;
class RemoteServerInterface;
class World;
class Character;
class Menu;
class Graphics;
class Sounds;
struct GraphicsConf;
class Stopwatch;
class BlockManager;
class ClientChunkManager;
class Save;
class Renderer;
struct Event;
class States;
class StateMachine;

class State;
class SystemInitState;
class PlayingState;
class LocalPlayingState;
class RemotePlayingState;

class Client {
public:
	enum StateId {
		CONNECTING,
		PLAYING,
		MENU,
	};

	Client(const char *worldId, const char *serverAdress);

	// getter
	bool isDebugOn() const { return debugOn; }
	const GraphicsConf &getConf() const { return *conf.get(); }
	StateId getStateId() const { return stateId; }

	// access
	Stopwatch *getStopwatch() { return stopwatch.get(); }
	Graphics *getGraphics() { return graphics.get(); }
	Sounds *getSounds() { return sounds.get(); }
	Menu *getMenu() { return menu.get(); }
	Save *getSave() { return save.get(); }
	BlockManager *getBlockManager() { return blockManager.get(); }
	ClientChunkManager *getChunkManager() { return chunkManager.get(); }
	World *getWorld() { return world.get(); }
	Renderer *getRenderer() { return renderer.get(); }
	ServerInterface *getServerInterface() { return serverInterface.get(); }
	States *getStates() { return states.get(); }
	StateMachine *getStateMachine() { return stateMachine.get(); }

	void setDebugOn(bool b) { debugOn = b; }
	void setConf(const GraphicsConf &);
	void setStateId(StateId id) { stateId = id; }

	// convenience functions
	uint8 getLocalClientId() const;
	Character &getLocalCharacter();

	// operation
	void run();

private:
	friend SystemInitState;
	friend PlayingState;
	friend LocalPlayingState;
	friend RemotePlayingState;

	std::unique_ptr<Stopwatch> stopwatch;
	std::unique_ptr<GraphicsConf> conf;
	std::unique_ptr<Graphics> graphics;
	std::unique_ptr<Sounds> sounds;
	std::unique_ptr<Menu> menu;
	std::unique_ptr<Save> save; 
	std::unique_ptr<BlockManager> blockManager;
	std::unique_ptr<ClientChunkManager> chunkManager;
	std::unique_ptr<World> world;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<ServerInterface> serverInterface;
	
	std::unique_ptr<States> states;
	std::unique_ptr<StateMachine> stateMachine;
	StateId stateId = StateId::CONNECTING;

	bool debugOn = false;

	Time time = 0;
    Time timeShift = 0;

	bool closeRequested = false;

private:
	void startGame();

	void sync(int perSecond);
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
