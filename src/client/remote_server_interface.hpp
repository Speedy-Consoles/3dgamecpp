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
	uint8 localPlayerId;
	Client *client;

	Time timeout = seconds(10); // 10 seconds

	boost::asio::io_service ios;
	boost::asio::io_service::work *w;
	std::future<void> f;
	Socket socket;

	std::future<void> connectFuture;
	std::atomic<Status> status;

	Buffer inBuf;
	Buffer outBuf;

	int moveInput = 0;

public:
	RemoteServerInterface(Client *client, const char *address);

	~RemoteServerInterface();

	Status getStatus() override;

	void toggleFly() override;

	void setPlayerMoveInput(int moveInput) override;

	void setPlayerOrientation(float yaw, float pitch) override;
	void setSelectedBlock(uint8 block) override;

	void placeBlock(vec3i64 bc, uint8 type) override;

	void tick() override;
	void run() override;

	void setConf(const GraphicsConf &, const GraphicsConf &) override;

	int getLocalClientId() override;

	void requestChunk(vec3i64 cc) override {}
	Chunk *getNextChunk() override { return nullptr; }

private:
	void asyncConnect(std::string address);
};

#endif // REMOTE_SERVER_INTERFACE_HPP

