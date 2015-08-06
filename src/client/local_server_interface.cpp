#include "local_server_interface.hpp"

#include <thread>

#include "engine/logging.hpp"

LocalServerInterface::LocalServerInterface(Client *client, uint64 seed) :
	client(client),
	archive((std::string("./") + client->getWorld()->getId() + "/").c_str()),
	worldGenerator(seed)
{
	client->getWorld()->addPlayer(0);
	player = &client->getWorld()->getPlayer(0);
}

LocalServerInterface::~LocalServerInterface() {
	client->getWorld()->deletePlayer(0);
}

ServerInterface::Status LocalServerInterface::getStatus() {
	return CONNECTED;
}

int LocalServerInterface::getLocalClientId() {
	return 0;
}

void LocalServerInterface::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	// nothing
}

void LocalServerInterface::receive() {
	// nothing
}

void LocalServerInterface::send() {
	// nothing
}

void LocalServerInterface::setPlayerMoveInput(int moveInput) {
	if (player->isValid())
		player->setMoveInput(moveInput);
}

void LocalServerInterface::setPlayerOrientation(double yaw, double pitch) {
	player->setOrientation(yaw, pitch);
}

void LocalServerInterface::setSelectedBlock(uint8 block) {
	player->setBlock(block);
}

void LocalServerInterface::placeBlock(vec3i64 bc, uint8 type) {
	//TODO

}

void LocalServerInterface::toggleFly() {
	player->setFly(!player->getFly());
}

void LocalServerInterface::requestChunk(vec3i64 cc) {
	Chunk *chunk = new Chunk(cc);
	if (!chunk) {
		LOG(ERROR, "Chunk allocation failed");
	}
	if (!archive.loadChunk(*chunk)) {
		worldGenerator.generateChunk(*chunk);
		archive.storeChunk(*chunk);
	}
	chunkQueue.push(chunk);
}

Chunk *LocalServerInterface::getNextChunk() {
	Chunk *chunk = chunkQueue.front();
	chunkQueue.pop();
	return chunk;
}
