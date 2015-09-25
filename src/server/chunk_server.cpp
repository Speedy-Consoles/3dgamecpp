#include "chunk_server.hpp"

#include "shared/chunk_compression.hpp"

static logging::Logger logger("cserver");

ChunkServer::ChunkServer(Server *server) : server(server), toEncodeQueue(1024), encodedQueue(1024) {
	LOG_INFO(logger) << "Creating chunk server";
	chunkManager = server->getChunkManager();
}

ChunkServer::~ChunkServer() {
	ChunkMessageJob job;
	while (encodedQueue.pop(job));
	wait();
}

void ChunkServer::tick() {
	ChunkMessageJob job;
	while(!requestedQueue.empty()) {
		job = requestedQueue.front();
		const Chunk *chunk = chunkManager->getChunk(job.request.coords);
		if (!chunk)
			break;
		job.blocks = chunk->getBlocks();
		job.message.chunkCoords = job.request.coords;
		job.message.revision = chunk->getRevision();
		if (chunk->getRevision() == job.request.cachedRevision)
			job.message.encodedLength = 0;
		else if (!toEncodeQueue.push(job))
			break;
	}

	while (encodedQueue.pop(job)) {
		server->finishChunkMessageJob(job);
	}
}

void ChunkServer::onChunkRequest(ChunkMessageJob job) {
	// TODO check for revision first
	chunkManager->requestChunk(job.request.coords);
	requestedQueue.push(job);
}

void ChunkServer::doWork() {
	ChunkMessageJob job;
	if (toEncodeQueue.pop(job)) {
		// TODO make this thread-safe
		encodeBlocks_RLE(job.blocks, (uint8 *) job.message.encodedBlocks, job.packet->dataLength);
		while (!encodedQueue.push(job)) {
			sleepFor(millis(50));
		}
	} else {
		sleepFor(millis(100));
	}
}
