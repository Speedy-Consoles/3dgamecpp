#ifndef REMOTE_SERVER_INTERFACE_HPP
#define REMOTE_SERVER_INTERFACE_HPP

#include "server_interface.hpp"

class RemoteServerInterface : public ServerInterface {
private:
	GraphicsConf conf;

public:
	RemoteServerInterface();

	virtual ~RemoteServerInterface();

	void togglePlayerFly() override;

	void setPlayerMoveInput(int moveInput) override;

	void setPlayerOrientation(double yaw, double pitch) override;
	void setBlock(uint8 block) override;

	void edit(vec3i64 bc, uint8 type) override;

	void receive(uint64 timeLimit) override;

	void sendInput() override;

	void setConf(const GraphicsConf &) override;

	int getLocalClientID() override;

	void stop() override;
};

#endif // REMOTE_SERVER_INTERFACE_HPP

