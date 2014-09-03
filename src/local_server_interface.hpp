#ifndef LOCAL_SERVER_INTERFACE_HPP
#define LOCAL_SERVER_INTERFACE_HPP

#include "server_interface.hpp"
#include "world.hpp"
#include "chunk_loader.hpp"

class LocalServerInterface : public ServerInterface {
private:
	World *world;
	ChunkLoader chunkLoader;

public:
	LocalServerInterface(World *world, uint64 seed);

	virtual ~LocalServerInterface();

	void togglePlayerFly() override;

	void setPlayerMoveInput(int moveInput) override;

	void setPlayerOrientation(double yaw, double pitch) override;

	void edit(vec3i64 bc, uint8 type) override;

	void receive(uint64 timeLimit) override;

	void sendInput() override;

	int getLocalClientID() override;

	void stop() override;
};

#endif // LOCAL_SERVER_INTERFACE_HPP

