#ifndef LOCAL_SERVER_INTERFACE_HPP
#define LOCAL_SERVER_INTERFACE_HPP

#include "server_interface.hpp"
#include "game/world.hpp"
#include "game/world_generator.hpp"
#include "io/chunk_loader.hpp"
#include "client.hpp"

class LocalServerInterface : public ServerInterface {
	Client *client;
	Player *player;

	WorldGenerator worldGenerator;
	ChunkArchive archive;

	std::queue<vec3i64> preToLoadQueue;
	ProducerQueue<Chunk *> loadedQueue;
	ProducerQueue<vec3i64> toLoadQueue;

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
	void requestChunk(vec3i64 cc) override;
	Chunk *getNextChunk() override;
};

#endif // LOCAL_SERVER_INTERFACE_HPP

