#include "local_server_interface.hpp"

LocalServerInterface::LocalServerInterface(World *world, uint64 seed)
		: chunkLoader(world, seed, true) {
	this->world = world;
	//chunkLoader.start();
	world->addPlayer(0);
	chunkLoader.run();
}

void LocalServerInterface::togglePlayerFly() {
	world->getPlayer(0).setFly(!world->getPlayer(0).getFly());
}

void LocalServerInterface::setPlayerMoveInput(int moveInput) {
	//		world.getPlsayer(0).setMoveInput(moveInput);
}

void LocalServerInterface::setPlayerOrientation(double yaw, double pitch) {
	//		world.getPlayer(0).setOrientation(yaw, pitch);
}

void LocalServerInterface::edit(vec3i64 bc, uint8 type) {
	world->setBlock(bc, type, true);
}

void LocalServerInterface::receive(uint64 timeLimit) {
}

void LocalServerInterface::sendInput() {
}

int LocalServerInterface::getLocalClientID() {
	return 0;
}

void LocalServerInterface::stop() {
	world->deletePlayer(0);
	//chunkLoader.interrupt();
}

