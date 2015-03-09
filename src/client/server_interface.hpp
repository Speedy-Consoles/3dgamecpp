#ifndef SERVER_INTERFACE_HPP
#define SERVER_INTERFACE_HPP

#include "engine/vmath.hpp"
#include "config.hpp"

class ServerInterface {
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

	virtual ~ServerInterface() {}

	virtual Status getStatus() = 0;

	virtual void toggleFly() = 0;
	virtual void setPlayerMoveInput(int moveInput) = 0;
	virtual void setPlayerOrientation(double yaw, double pitch) = 0;
	virtual void setBlock(uint8 block) = 0;

	virtual void edit(vec3i64 block, uint8 type) = 0;

	virtual void receive(uint64 timeLimit) = 0;

	virtual void sendInput() = 0;

	virtual void setConf(const GraphicsConf &) = 0;

	virtual int getLocalClientId() = 0;

	virtual void stop() = 0;
};

#endif // SERVER_INTERFACE_HPP
