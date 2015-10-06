#include "chunk_server.hpp"

#include "shared/chunk_compression.hpp"

static logging::Logger logger("cserver");

ChunkServer::ChunkServer(Server *server) : server(server) {
	LOG_INFO(logger) << "Creating chunk server";
	chunkManager = server->getChunkManager();
}

ChunkServer::~ChunkServer() {
	// nothing
}

void ChunkServer::tick() {
	while(!requestedQueue.empty()) {
		TaggedChunkRequest tcr = requestedQueue.front();
		const Chunk *chunk = chunkManager->getChunk(tcr.request.coords);
		if (!chunk)
			break;
		requestedQueue.pop();
		ChunkMessage message;
		message.chunkCoords = tcr.request.coords;
		message.revision = chunk->getRevision();
		if (chunk->getRevision() != tcr.request.cachedRevision) {
			// TODO allocation should happen in chunk manager
			message.encodedBlocks = new uint8[Chunk::SIZE];
			message.encodedLength = encodeBlocks_RLE(chunk->getBlocks(), message.encodedBlocks, Chunk::SIZE);
			server->send(message, tcr.clientId, CHANNEL_BLOCK_DATA, true);
			// TODO deallocation should also happen in chunk manager
			delete[] message.encodedBlocks;
		} else {
			message.encodedLength = 0;
			server->send(message, tcr.clientId, CHANNEL_BLOCK_DATA, true);
		}
		chunkManager->releaseChunk(tcr.request.coords);
	}
}

void ChunkServer::onClientLeave(int id) {
	std::queue<TaggedChunkRequest> newQueue;
	while(!requestedQueue.empty()) {
		TaggedChunkRequest tcr = requestedQueue.front();
		if (tcr.clientId != id)
			newQueue.push(tcr);
		requestedQueue.pop();
	}
}

void ChunkServer::onChunkRequest(ChunkRequest request, int clientId) {
	// TODO check for revision first
	// TODO request compressed blocks instead of whole chunk
	chunkManager->requireChunk(request.coords);
	requestedQueue.push(TaggedChunkRequest{clientId, request});
}
