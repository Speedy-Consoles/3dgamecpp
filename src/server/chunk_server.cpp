#include "chunk_server.hpp"

static logging::Logger logger("cserver");

ChunkServer::ChunkServer(Server *server) : server(server) {
	LOG_INFO(logger) << "Creating chunk server";
	chunkManager = server->getChunkManager();
}

ChunkServer::~ChunkServer() {
	// nothing
}

void ChunkServer::tick() {
	chunkManager->tick();
	// TODO
}

void ChunkServer::onChunkRequest(ChunkRequest &request, ChunkMessageJob job) {
	// TODO
}
