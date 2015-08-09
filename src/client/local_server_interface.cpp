#include "local_server_interface.hpp"

#include <thread>
#include <atomic>

#include "graphics.hpp"

#include "engine/logging.hpp"
static logging::Logger logger("local");

using namespace std;

LocalServerInterface::LocalServerInterface(Client *client, uint64 seed) :
	client(client),
	worldGenerator(seed),
	loadedQueue(1024),
	toLoadQueue(1024)
{
	client->getWorld()->addPlayer(0);
	player = &client->getWorld()->getPlayer(0);
}

LocalServerInterface::~LocalServerInterface() {
	client->getWorld()->deletePlayer(0);
	wait();
	Chunk *chunk;
	while (loadedQueue.pop(chunk)) {
		delete chunk;
	}
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

void LocalServerInterface::doWork() {
	vec3i64 cc;
	if (toLoadQueue.pop(cc)) {
		Chunk *chunk = new Chunk(cc);
		if (!chunk) {
			LOG_ERROR(logger) << "Chunk allocation failed";
		}
		worldGenerator.generateChunk(*chunk);
		while (!loadedQueue.push(chunk)) {
			sleepFor(millis(50));
		}
	} else {
		sleepFor(millis(100));
	}
}

void LocalServerInterface::setPlayerMoveInput(int moveInput) {
	if (player->isValid())
		player->setMoveInput(moveInput);
}

void LocalServerInterface::setPlayerOrientation(float yaw, float pitch) {
	player->setOrientation(yaw, pitch);
}

void LocalServerInterface::setSelectedBlock(uint8 block) {
	player->setBlock(block);
}

void LocalServerInterface::placeBlock(vec3i64 blockCoords, uint8 blockType) {
	client->getChunkManager()->placeBlock(blockCoords, blockType);
	// TODO tell world
	// TODO maybe move this to graphics or something
	vec3i64 cc = bc2cc(blockCoords);
	vec3ui8 icc = bc2icc(blockCoords);
	client->getGraphics()->getRenderer()->rerenderChunk(cc);
	for (size_t i = 0; i < 27; i++) {
		if (i == BIG_CUBE_CYCLE_BASE_INDEX)
			continue;
		bool rerender = true;
		for (int d = 0; d < 3 && rerender == true; d++) {
			switch (BIG_CUBE_CYCLE[i][d]) {
				case 0:
					break;
				case -1:
					if (icc[d] != 0)
						rerender = false;
					break;
				case 1:
					if (icc[d] != Chunk::WIDTH - 1)
						rerender = false;
					break;
				default:
					rerender = false;
					break;
			}
		}
		if (rerender)
			client->getGraphics()->getRenderer()->rerenderChunk(cc + BIG_CUBE_CYCLE[i].cast<int64>());
	}
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
