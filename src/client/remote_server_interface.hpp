#ifndef REMOTE_SERVER_INTERFACE_HPP
#define REMOTE_SERVER_INTERFACE_HPP

#include "server_interface.hpp"

#include <future>
#include <string>
#include <enet/enet.h>
#include <queue>

#include "shared/engine/time.hpp"
#include "shared/engine/logging.hpp"
#include "shared/game/world.hpp"
#include "shared/game/world_generator.hpp"
#include "shared/async_world_generator.hpp"
#include "shared/net.hpp"

class RemoteServerInterface : public ServerInterface {
private:
	uint8 localCharacterId = -1;
	Client *client = nullptr;

	std::unordered_map<vec3i64, Chunk *, size_t(*)(vec3i64)> requestedChunks;
	std::queue<Chunk *> receivedChunks;

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

	void setCharacterOrientation(int yaw, int pitch) override;
	void setSelectedBlock(uint8 block) override;

	void placeBlock(vec3i64 bc, uint8 type) override;

	void tick() override;

	void setConf(const GraphicsConf &, const GraphicsConf &) override;

	int getLocalClientId() override;

	bool requestChunk(Chunk *chunk, bool cached, uint32 cachedRevision) override;
	Chunk *getNextChunk() override;

private:
	void updateNet();
	void updateNetConnecting();
	void updateNetDisconnecting();

	void handlePacket(const enet_uint8 *data, size_t size, size_t channel);

	// TODO think about channels
	// maybe chat and events in different channels
	template<typename T> void send(T &msg, bool reliable) {
		static logging::Logger logger("remote");
		size_t size = getMessageSize(msg);
		enet_uint32 flags = 0;
		if (reliable)
			flags |= ENET_PACKET_FLAG_RELIABLE;
		ENetPacket *packet = enet_packet_create(nullptr, size, flags);
		if (writeMessage(msg, (char *) packet->data, size))
			LOG_ERROR(logger) << "Could not serialize message";

		enet_peer_send(peer, 0, packet);
	}
};

#endif // REMOTE_SERVER_INTERFACE_HPP

