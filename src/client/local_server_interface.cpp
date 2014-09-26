#include "local_server_interface.hpp"

#include <thread>

#include "io/logging.hpp"

LocalServerInterface::LocalServerInterface(World *world, uint64 seed, const GraphicsConf &conf)
		: conf(conf) {
	this->world = world;
	this->world->addPlayer(0);
	chunkLoader = new ChunkLoader(world, seed, getLocalClientID());
	chunkLoader->setRenderDistance(conf.render_distance);
	chunkLoader->dispatch();
}


LocalServerInterface::~LocalServerInterface() {
	delete chunkLoader;
}

ServerInterface::Status LocalServerInterface::getStatus() {
	return CONNECTED;
}

void LocalServerInterface::togglePlayerFly() {
	// TODO move this to client.cpp
	if (!world->isPaused())
		world->getPlayer(0).setFly(!world->getPlayer(0).getFly());
}

void LocalServerInterface::setPlayerMoveInput(int moveInput) {

}

void LocalServerInterface::setPlayerOrientation(double yaw, double pitch) {

}

void LocalServerInterface::setBlock(uint8 block) {

}

void LocalServerInterface::edit(vec3i64 bc, uint8 type) {
	// TODO move this to client.cpp
	if (!world->isPaused())
		world->setBlock(bc, type, true);
}

void LocalServerInterface::receiveChunks(uint64 timeLimit) {
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

int LocalServerInterface::getLocalClientID() {
	return 0;
}

void LocalServerInterface::stop() {
	chunkLoader->wait();
	world->deletePlayer(0);
}

