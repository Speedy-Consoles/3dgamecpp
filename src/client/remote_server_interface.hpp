#ifndef REMOTE_SERVER_INTERFACE_HPP
#define REMOTE_SERVER_INTERFACE_HPP

#include "server_interface.hpp"

#include <future>
#include <string>
#include <enet/enet.h>

#include "shared/engine/time.hpp"
#include "shared/engine/socket.hpp"
#include "shared/engine/buffer.hpp"
#include "shared/game/world.hpp"
#include "shared/game/world_generator.hpp"
#include "shared/async_world_generator.hpp"
#include "shared/net.hpp"

class RemoteServerInterface : public ServerInterface {
private:
	uint8 localPlayerId;
	Client *client;

	std::unique_ptr<WorldGenerator> worldGenerator;
	AsyncWorldGenerator asyncWorldGenerator;

	Status status = NOT_CONNECTED;

	ENetHost *host = nullptr;
	ENetPeer *peer = nullptr;

	Buffer inBuffer;
	Buffer outBuffer;

	int yaw = 0;
	int pitch = 0;
	int moveInput = 0;
	bool flying = false;

public:
	RemoteServerInterface(Client *client, std::string address);

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

	void receive(ServerMessage *cmsg, const enet_uint8 *data, size_t size);
	void send(ClientMessage &cmsg);
};

#endif // REMOTE_SERVER_INTERFACE_HPP

