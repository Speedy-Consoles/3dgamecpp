#ifndef SERVER_INTERFACE_HPP
#define SERVER_INTERFACE_HPP

#include <atomic>
#include <future>

#include "game/chunk.hpp"
#include "engine/vmath.hpp"
#include "engine/queue.hpp"
#include "config.hpp"

class ServerInterface {
	std::future<void> fut;

protected:
	std::atomic<bool> shouldHalt;

public:
	enum Status {
		NOT_CONNECTED,
		RESOLVING,
		COULD_NOT_RESOLVE,
		CONNECTING,
		SERVER_FULL,
		TIMEOUT,
		CONNECTION_ERROR,
		CONNECTED,
	};

	virtual ~ServerInterface() = default;

	// threading
	void dispatch();
	void requestTermination();
	void wait();
	virtual void run() = 0;

	// query
	virtual Status getStatus() = 0;
	virtual int getLocalClientId() = 0;

	virtual void setConf(const GraphicsConf &, const GraphicsConf &) {};

	// networking
	virtual void tick() = 0;

	// player actions
	virtual void setPlayerMoveInput(int moveInput) = 0;
	virtual void setPlayerOrientation(double yaw, double pitch) = 0;

	virtual void setSelectedBlock(uint8 block) = 0;
	virtual void placeBlock(vec3i64 block, uint8 type) = 0;
	virtual void toggleFly() = 0;

	// chunks
	virtual void requestChunk(vec3i64 cc) = 0;
	virtual Chunk *getNextChunk() = 0;
};

#endif // SERVER_INTERFACE_HPP
