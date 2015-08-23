#ifndef LOCAL_SERVER_INTERFACE_HPP
#define LOCAL_SERVER_INTERFACE_HPP

#include <memory>

#include "server_interface.hpp"

#include "shared/game/world.hpp"
#include "shared/game/world_generator.hpp"

#include "client.hpp"

class LocalServerInterface : public ServerInterface {
	Client *client;
	Player *player;

	std::unique_ptr<WorldGenerator> worldGenerator;

	ProducerQueue<Chunk *> loadedQueue;
	ProducerQueue<Chunk *> toLoadQueue;

public:
	LocalServerInterface(Client * client, uint64 seed);
	~LocalServerInterface();

	// query
	Status getStatus() override;
	int getLocalClientId() override;

	void setConf(const GraphicsConf &, const GraphicsConf &) override;

	// networking
	void tick() override;
	void doWork() override;

	// player actions
	void setPlayerMoveInput(int moveInput) override;
	void setPlayerOrientation(float yaw, float pitch) override;

	void setSelectedBlock(uint8 block) override;
	void placeBlock(vec3i64 bc, uint8 type) override;
	void toggleFly() override;
	
	// chunks
	bool requestChunk(Chunk *chunk) override;
	Chunk *getNextChunk() override;
};

#endif // LOCAL_SERVER_INTERFACE_HPP

