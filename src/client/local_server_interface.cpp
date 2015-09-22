#include "local_server_interface.hpp"

#include <thread>
#include <atomic>

#include "shared/engine/logging.hpp"
#include "shared/saves.hpp"
#include "client/gfx/graphics.hpp"
#include "client_chunk_manager.hpp"

using namespace std;

static logging::Logger logger("local");

LocalServerInterface::LocalServerInterface(Client *client) :
	client(client),
	worldGenerator(client->getSave()->getWorldGenerator()),
	asyncWorldGenerator(worldGenerator.get())
{
	client->getWorld()->addCharacter(0);
	character = &client->getWorld()->getCharacter(0);

	vec3i64 spawnBC = client->getSave()->getSpawn();
	vec3i64 spawnWC = spawnBC * RESOLUTION;
	spawnWC += vec3i64(RESOLUTION / 2, RESOLUTION / 2, Character::EYE_HEIGHT);
	character->setPos(spawnWC);
}

LocalServerInterface::~LocalServerInterface() {
	client->getWorld()->deleteCharacter(0);
}

ServerInterface::Status LocalServerInterface::getStatus() {
	return CONNECTED;
}

int LocalServerInterface::getLocalClientId() {
	return 0;
}

void LocalServerInterface::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.fog == old.fog)
		return;
	else
		return;
}

void LocalServerInterface::tick() {
	client->getWorld()->tick();
}

void LocalServerInterface::setPlayerMoveInput(int moveInput) {
	if (character->isValid())
		character->setMoveInput(moveInput);
}

void LocalServerInterface::setCharacterOrientation(int yaw, int pitch) {
	if (character->isValid())
		character->setOrientation(yaw, pitch);
}

void LocalServerInterface::setSelectedBlock(uint8 block) {
	if (character->isValid())
		character->setBlock(block);
}

void LocalServerInterface::placeBlock(vec3i64 blockCoords, uint8 blockType) {
	vec3i64 cc = bc2cc(blockCoords);
	vec3ui8 icc = bc2icc(blockCoords);
	const Chunk *chunk = client->getChunkManager()->getChunk(cc);
	if (chunk) {
		size_t blockIndex = Chunk::getBlockIndex(icc);
		client->getChunkManager()->placeBlock(
			cc,
			blockIndex,
			blockType,
			chunk->getRevision()
		);
	}
	// TODO tell world
	// TODO maybe move this to graphics or something
	client->getRenderer()->rebuildChunk(cc);
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
			client->getRenderer()->rebuildChunk(cc + BIG_CUBE_CYCLE[i].cast<int64>());
	}
}

void LocalServerInterface::toggleFly() {
	character->setFly(!character->getFly());
}

bool LocalServerInterface::requestChunk(Chunk *chunk) {
	return asyncWorldGenerator.requestChunk(chunk);
}

Chunk *LocalServerInterface::getNextChunk() {
	return asyncWorldGenerator.getNextChunk();
}
