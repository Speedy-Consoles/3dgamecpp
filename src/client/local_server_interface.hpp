#ifndef LOCAL_SERVER_INTERFACE_HPP
#define LOCAL_SERVER_INTERFACE_HPP

#include <memory>

#include "server_interface.hpp"

#include "shared/game/world.hpp"
#include "shared/game/world_generator.hpp"
#include "shared/async_world_generator.hpp"

#include "client.hpp"

class LocalServerInterface : public ServerInterface {
	Client *client;
	Character *character;

	std::unique_ptr<WorldGenerator> worldGenerator;
	AsyncWorldGenerator asyncWorldGenerator;

public:
	LocalServerInterface(Client * client);
	~LocalServerInterface();

	// query
	Status getStatus() override;
	int getLocalClientId() override;

	void setConf(const GraphicsConf &, const GraphicsConf &) override;

	// networking
	void tick() override;

	// player actions
	void setPlayerMoveInput(int moveInput) override;
	void setCharacterOrientation(int yaw, int pitch) override;

	void setSelectedBlock(uint8 block) override;
	void placeBlock(vec3i64 bc, uint8 type) override;
	void toggleFly() override;
	
	// chunks
	bool requestChunk(Chunk *chunk) override;
	Chunk *getNextChunk() override;
};

#endif // LOCAL_SERVER_INTERFACE_HPP

