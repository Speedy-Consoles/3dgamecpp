#ifndef SERVER_INTERFACE_HPP
#define SERVER_INTERFACE_HPP

#include "shared/engine/vmath.hpp"
#include "shared/engine/queue.hpp"
#include "shared/engine/thread.hpp"
#include "shared/game/chunk.hpp"

#include "config.hpp"

class ServerInterface {
public:
	enum Status {
		NOT_CONNECTED,
		CONNECTING,
		CONNECTION_ERROR,
		WAITING_FOR_SNAPSHOT,
		CONNECTED,
		DISCONNECTING,
	};
	
	ServerInterface() {}
	virtual ~ServerInterface() = default;

	// query
	virtual Status getStatus() = 0;
	virtual int getLocalClientId() = 0;

	virtual void setConf(const GraphicsConf &, const GraphicsConf &) {};

	// networking
	virtual void tick() = 0;

	// player actions
	virtual void setPlayerMoveInput(int moveInput) = 0;
	virtual void setCharacterOrientation(int yaw, int pitch) = 0;

	virtual void setSelectedBlock(uint8 block) = 0;
	virtual void placeBlock(vec3i64 block, uint8 type) = 0;
	virtual void toggleFly() = 0;

	// chunks
	virtual bool requestChunk(Chunk *chunk) = 0;
	virtual Chunk *getNextChunk() = 0;
};

#endif // SERVER_INTERFACE_HPP
