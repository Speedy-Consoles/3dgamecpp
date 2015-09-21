#ifndef REMOTE_SERVER_INTERFACE_HPP
#define REMOTE_SERVER_INTERFACE_HPP

#include "server_interface.hpp"

#include <future>
#include <string>
#include <enet/enet.h>

#include "shared/engine/time.hpp"
#include "shared/engine/logging.hpp"
#include "shared/game/world.hpp"
#include "shared/game/world_generator.hpp"
#include "shared/async_world_generator.hpp"
#include "shared/net.hpp"

class RemoteServerInterface : public ServerInterface {
private:
	uint8 localPlayerId = -1;
	Client *client = nullptr;

	std::unique_ptr<WorldGenerator> worldGenerator;
	AsyncWorldGenerator asyncWorldGenerator;

	Status status = NOT_CONNECTED;

	ENetHost *host = nullptr;
	ENetPeer *peer = nullptr;

	int yaw = 0;
	int pitch = 0;
	int moveInput = 0;
	bool flying = false;

public:
	RemoteServerInterface(Client *client, std::string addressString);
	~RemoteServerInterface();

	Status getStatus() override;

	void toggleFly() override;

	void setPlayerMoveInput(int moveInput) override;

	void setPlayerOrientation(int yaw, int pitch) override;
	void setSelectedBlock(uint8 block) override;

	void placeBlock(vec3i64 bc, uint8 type) override;

	void tick() override;

	void setConf(const GraphicsConf &, const GraphicsConf &) override;

	int getLocalClientId() override;

	bool requestChunk(Chunk *chunk) override;
	Chunk *getNextChunk() override;

private:
	void handlePacket(const enet_uint8 *data, size_t size, size_t channel);

	template<typename T> void send(T &msg) {
		static logging::Logger logger("remote");
		// TODO reliability, order
		size_t size = getMessageSize(msg);
		ENetPacket *packet = enet_packet_create(nullptr, size, 0);
		if (writeMessage(msg, (char *) packet->data, size))
			LOG_ERROR(logger) << "Could not serialize message";

		enet_peer_send(peer, 0, packet);
	}
};

#endif // REMOTE_SERVER_INTERFACE_HPP

