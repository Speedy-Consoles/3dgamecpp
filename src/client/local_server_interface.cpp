#include "local_server_interface.hpp"

#include <thread>

#include "engine/logging.hpp"

LocalServerInterface::LocalServerInterface(World *world, uint64 seed, const GraphicsConf &conf)
		: conf(conf) {
	this->world = world;
	this->world->addPlayer(0);
	player = &world->getPlayer(0);
	chunkLoader = new ChunkLoader(world, seed, getLocalClientId());
	chunkLoader->setRenderDistance(conf.render_distance);
	chunkLoader->dispatch();
}


LocalServerInterface::~LocalServerInterface() {
	delete chunkLoader;
}

ServerInterface::Status LocalServerInterface::getStatus() {
	return CONNECTED;
}

void LocalServerInterface::toggleFly() {
	player->setFly(!player->getFly());
}

void LocalServerInterface::setPlayerMoveInput(int moveInput) {
	if (player->isValid())
		player->setMoveInput(moveInput);
}

void LocalServerInterface::setPlayerOrientation(double yaw, double pitch) {
	player->setOrientation(yaw, pitch);
}

void LocalServerInterface::setBlock(uint8 block) {
	player->setBlock(block);
}

void LocalServerInterface::edit(vec3i64 bc, uint8 type) {
	world->setBlock(bc, type, true);
}

void LocalServerInterface::receive(uint64 timeLimit) {
	Chunk *chunk = nullptr;
	while ((chunk = chunkLoader->getNextLoadedChunk()) != nullptr) {
		world->insertChunk(chunk);
	}

	auto unloadQueries = chunkLoader->getUnloadQueries();
	while (unloadQueries)
	{
		Chunk *chunk = world->removeChunk(unloadQueries->data);
		chunk->free();
		auto tmp = unloadQueries->next;
		delete unloadQueries;
		unloadQueries = tmp;
	}
}

void LocalServerInterface::sendInput() {

}

void LocalServerInterface::setConf(const GraphicsConf &conf) {
	GraphicsConf old_conf = this->conf;
	this->conf = conf;

	if (conf.render_distance != old_conf.render_distance) {
		chunkLoader->setRenderDistance(conf.render_distance);
		world->clearChunks();
		while (chunkLoader->getRenderDistance() != conf.render_distance)
			std::this_thread::yield();
		LOG(INFO, "render distance was set by chunk loader");
	}
}

int LocalServerInterface::getLocalClientId() {
	return 0;
}

void LocalServerInterface::stop() {
	chunkLoader->wait();
	world->deletePlayer(0);
}

