#include "chunk_manager.hpp"
#include "engine/logging.hpp"
#include "engine/time.hpp"
#include "client/client.hpp"
#include "client/server_interface.hpp"
#include "game/world.hpp"

using namespace std;

static logging::Logger logger("chm");

ChunkManager::ChunkManager(Client *client) :
	loadedStoredQueue(1024),
	toLoadStoreQueue(1024),
	chunks(0, vec3i64HashFunc),
	needCounter(0, vec3i64HashFunc),
	client(client),
	archive("./region/")
{
	// nothing
}

ChunkManager::~ChunkManager() {
	// TODO delete all chunks
}

void ChunkManager::tick() {
	while (!requestedQueue.empty() && numAllocatedChunks < MAX_ALLOCATED_CHUNKS) {
		vec3i64 cc = requestedQueue.front();
		Chunk *chunk = new Chunk(cc);
		numAllocatedChunks++;
		if (!chunk)
			LOG_ERROR(logger) << "Chunk allocation failed";
		ArchiveOperation op = {chunk, LOAD};
		if (toLoadStoreQueue.push(op)) {
			requestedQueue.pop();
			continue;
		} else {
			delete chunk;
			numAllocatedChunks--;
			break;
		}
	}

	while (!preToStoreQueue.empty()) {
		Chunk *chunk = preToStoreQueue.front();
		ArchiveOperation op = {chunk, STORE};
		if (toLoadStoreQueue.push(op)) {
			preToStoreQueue.pop();
			continue;
		} else
			break;
	}

	while(!notInCacheQueue.empty()) {
		Chunk *chunk = notInCacheQueue.front();
		if (client->getServerInterface()->requestChunk(chunk))
			notInCacheQueue.pop();
		else
			break;
	}

	ArchiveOperation op;
	while (loadedStoredQueue.pop(op)) {
		switch(op.type) {
		case LOAD:
			if (op.chunk->initialized) {
				insertLoadedChunk(op.chunk);
			} else {
				notInCacheQueue.push(op.chunk);
			}
			break;
		case STORE:
			delete op.chunk;
			numAllocatedChunks--;
			break;
		}
	}

	Chunk *chunk;
	while ((chunk = client->getServerInterface()->getNextChunk()) != nullptr) {
		if (!chunk->initialized)
			LOG_WARNING(logger) << "Server interface didn't initialize chunk";
		insertLoadedChunk(chunk);
	}
}

void ChunkManager::doWork() {
	ArchiveOperation op;
	if (toLoadStoreQueue.pop(op)) {
		switch (op.type) {
		case LOAD:
			archive.loadChunk(*op.chunk);
			break;
		case STORE:
			archive.storeChunk(*op.chunk);
			break;
		}
		while (!loadedStoredQueue.push(op)) {
			sleepFor(millis(50));
		}
	} else {
		sleepFor(millis(100));
	}
}

void ChunkManager::onStop() {
	ArchiveOperation op;
	while (toLoadStoreQueue.pop(op)) {
		if (op.type == STORE) {
			archive.storeChunk(*op.chunk);
		}
	}
}

void ChunkManager::storeChunks() {
	wait();
	for (auto it = chunks.begin(); it != chunks.end(); ++it) {
		archive.storeChunk(*it->second);
	}
}

void ChunkManager::placeBlock(vec3i64 blockCoords, uint8 blockType) {
	vec3i64 cc = bc2cc(blockCoords);
	auto it = chunks.find(cc);
	if (it != chunks.end())
		it->second->setBlock(bc2icc(blockCoords), blockType);
	// TODO operate on cache if chunk is not loaded
}

const Chunk *ChunkManager::getChunk(vec3i64 chunkCoords) const {
	auto it = chunks.find(chunkCoords);
	if (it != chunks.end())
		return it->second;
	return nullptr;
}

void ChunkManager::requestChunk(vec3i64 chunkCoords) {
	auto it = needCounter.find(chunkCoords);
	if (it == needCounter.end()) {
		requestedQueue.push(chunkCoords);
		needCounter.insert({chunkCoords, 1});
	} else {
		it->second++;
	}
}

void ChunkManager::releaseChunk(vec3i64 chunkCoords) {
	auto it1 = needCounter.find(chunkCoords);
	if (it1 != needCounter.end()) {
		it1->second--;
		if (it1->second == 0) {
			needCounter.erase(it1);
			auto it2 = chunks.find(chunkCoords);
			if (it2 != chunks.end()) {
				preToStoreQueue.push(it2->second);
				chunks.erase(it2);
			}
		}
	}
}

int ChunkManager::getNumNeededChunks() const {
	return needCounter.size();
}

int ChunkManager::getNumAllocatedChunks() const {
	return numAllocatedChunks;
}

int ChunkManager::getNumLoadedChunks() const {
	return chunks.size();
}

int ChunkManager::getRequestedQueueSize() const {
	return requestedQueue.size();
}

int ChunkManager::getNotInCacheQueueSize() const {
	return notInCacheQueue.size();
}

void ChunkManager::insertLoadedChunk(Chunk *chunk) {
	auto it = needCounter.find(chunk->getCC());
	if (it != needCounter.end()) {
		chunk->makePassThroughs();
		chunks.insert({chunk->getCC(), chunk});
	} else {
		delete chunk;
		numAllocatedChunks--;
	}
}
