#ifndef REMOTE_SERVER_INTERFACE_HPP
#define REMOTE_SERVER_INTERFACE_HPP

#include "server_interface.hpp"

#include <future>
#include <string>

#include "shared/engine/time.hpp"
#include "shared/engine/socket.hpp"
#include "shared/engine/buffer.hpp"
#include "shared/game/world.hpp"
#include "shared/game/world_generator.hpp"
#include "shared/net.hpp"

class RemoteServerInterface : public ServerInterface {
private:
	uint8 localPlayerId;
	Client *client;

	std::unique_ptr<WorldGenerator> worldGenerator;
	ProducerQueue<Chunk *> loadedQueue;
	ProducerQueue<Chunk *> toLoadQueue;

	Time timeout = seconds(10); // 10 seconds

	boost::asio::io_service ios;
	boost::asio::io_service::work *w;
	std::future<void> f;
	Socket socket;

	std::future<void> connectFuture;
	std::atomic<Status> status;

	Buffer inBuf;
	Buffer outBuf;

	int yaw = 0;
	int pitch = 0;
	int moveInput = 0;
	bool flying = false;

public:
	RemoteServerInterface(Client *client, const char *address);

	~RemoteServerInterface();

	Status getStatus() override;

	void toggleFly() override;

	void setPlayerMoveInput(int moveInput) override;

	void setPlayerOrientation(int yaw, int pitch) override;
	void setSelectedBlock(uint8 block) override;

	void placeBlock(vec3i64 bc, uint8 type) override;

	void tick() override;
	void doWork() override;

	void setConf(const GraphicsConf &, const GraphicsConf &) override;

	int getLocalClientId() override;

	bool requestChunk(Chunk *chunk) override;
	Chunk *getNextChunk() override;

private:
	void asyncConnect(std::string address);
};

#endif // REMOTE_SERVER_INTERFACE_HPP

