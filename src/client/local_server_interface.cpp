#include "local_server_interface.hpp"

#include <thread>
#include <atomic>

#include "engine/logging.hpp"

using namespace std;

LocalServerInterface::LocalServerInterface(Client *client, uint64 seed) :
	client(client),
	archive((std::string("./") + client->getWorld()->getId() + "/").c_str()),
	worldGenerator(seed),
	loadedQueue(1024),
	toLoadQueue(1024)
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

void LocalServerInterface::tick() {
	while (!preToLoadQueue.empty()) {
		vec3i64 cc = preToLoadQueue.front();
		if (!toLoadQueue.push(cc))
			break;
		preToLoadQueue.pop();
	}
}

void LocalServerInterface::run() {
	LOG(INFO, "ChunkManager thread dispatched");

	while (!shouldHalt.load(memory_order_seq_cst)) {
		vec3i64 cc;
		if (toLoadQueue.pop(cc)) {
			Chunk *chunk = new Chunk(cc);
			if (!chunk) {
				LOG(ERROR, "Chunk allocation failed");
			}
			if (!archive.loadChunk(*chunk)) {
				worldGenerator.generateChunk(*chunk);
				archive.storeChunk(*chunk);
			}
			chunk->makePassThroughs();
			while (!loadedQueue.push(chunk) && !shouldHalt.load(memory_order_seq_cst)) {
				sleepFor(millis(50));
			}
		} else {
			sleepFor(millis(100));
		}
	} // while not thread interrupted

	LOG(INFO,"ChunkManager thread terminating");
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

void LocalServerInterface::requestChunk(vec3i64 chunkCoords) {
	preToLoadQueue.push(chunkCoords);
}

Chunk *LocalServerInterface::getNextChunk() {
	Chunk *chunk = nullptr;
	loadedQueue.pop(chunk);
	return chunk;
}
