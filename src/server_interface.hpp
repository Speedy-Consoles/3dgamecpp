#ifndef SERVER_INTERFACE_HPP
#define SERVER_INTERFACE_HPP

#include "vmath.hpp"

class ServerInterface {
public:
	virtual ~ServerInterface() {}

	virtual void togglePlayerFly() = 0;

	virtual void setPlayerMoveInput(int moveInput) = 0;

	virtual void setPlayerOrientation(double yaw, double pitch) = 0;

	virtual void edit(vec3i64 block, uint8 type) = 0;

	virtual void receive(uint64 timeLimit) = 0;

	virtual void sendInput() = 0;

	virtual void setConf(const GraphicsConf &) = 0;

	virtual int getLocalClientID() = 0;

	virtual void stop() = 0;
};

#endif // SERVER_INTERFACE_HPP
