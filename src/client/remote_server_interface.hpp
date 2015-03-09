#ifndef REMOTE_SERVER_INTERFACE_HPP
#define REMOTE_SERVER_INTERFACE_HPP

#include <future>
#include <string>

#include "server_interface.hpp"
#include "engine/time.hpp"
#include "engine/net.hpp"
#include "engine/socket.hpp"
#include "engine/buffer.hpp"
#include "game/world.hpp"

class RemoteServerInterface : public ServerInterface {
private:
	GraphicsConf conf;
	uint8 localPlayerId;
	World *world;

	Time timeout = seconds(10); // 10 seconds

	ios_t ios;
	ios_t::work *w;
	std::future<void> f;
	Socket socket;

	std::future<void> connectFuture;
	std::atomic<Status> status;

	Buffer inBuf;
	Buffer outBuf;

	int moveInput = 0;

public:
	RemoteServerInterface(World *world, const char *address, const GraphicsConf &conf);

	~RemoteServerInterface();

	Status getStatus() override;

	void toggleFly() override;

	void setPlayerMoveInput(int moveInput) override;

	void setPlayerOrientation(double yaw, double pitch) override;
	void setBlock(uint8 block) override;

	void edit(vec3i64 bc, uint8 type) override;

	void receive(uint64 timeLimit) override;

	void sendInput() override;

	void setConf(const GraphicsConf &) override;

	int getLocalClientId() override;

	void stop() override;

private:
	void asyncConnect(std::string address);
};

#endif // REMOTE_SERVER_INTERFACE_HPP

