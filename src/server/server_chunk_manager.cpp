#include "server_chunk_manager.hpp"

#include "shared/engine/logging.hpp"
#include "shared/engine/time.hpp"
#include "shared/game/world.hpp"

using namespace std;

static logging::Logger logger("chm");

ServerChunkManager::ServerChunkManager(
		std::unique_ptr<WorldGenerator> worldGenerator,
		std::unique_ptr<ChunkArchive> archive) :
	threadOutQueue(1024),
	threadInQueue(1024),
	chunks(0, vec3i64HashFunc),
	cacheRevisions(0, vec3i64HashFunc),
	needCounter(0, vec3i64HashFunc),
	worldGenerator(std::move(worldGenerator)),
	asyncWorldGenerator(this->worldGenerator.get()),
	archive(std::move(archive))
{
	for (int i = 0; i < CHUNK_POOL_SIZE; i++) {
		chunkPool[i] = new Chunk(Chunk::ChunkFlags::VISUAL);
		unusedChunks.push(chunkPool[i]);
	}
	dispatch();
}

ServerChunkManager::~ServerChunkManager() {
	LOG_TRACE(logger) << "Destroying ChunkManager";
	requestTermination();
	ArchiveOperation op;
	while(threadOutQueue.pop(op));
	wait();
	while (!prethreadInQueue.empty()) {
		ArchiveOperation op = prethreadInQueue.front();
		prethreadInQueue.pop();
		if (op.type != STORE)
			continue;
		archive->storeChunk(*op.chunk);
	}
	for (auto it1 = chunks.begin(); it1 != chunks.end(); ++it1) {
		auto it2 = cacheRevisions.find(it1->first);
		if (it2 == cacheRevisions.end() || it1->second->getRevision() != it2->second)
			archive->storeChunk(*it1->second);
	}
	for (int i = 0; i < CHUNK_POOL_SIZE; i++) {
		delete chunkPool[i];
	}
}

void ServerChunkManager::tick() {
	while (!requestedQueue.empty() && !unusedChunks.empty()) {
		vec3i64 cc = requestedQueue.front();
		Chunk *chunk = unusedChunks.top();
		if (!chunk)
			LOG_ERROR(logger) << "Chunk allocation failed";
		chunk->initCC(cc);
		ArchiveOperation op = {chunk, LOAD};
		prethreadInQueue.push(op);
		requestedQueue.pop();
		unusedChunks.pop();
	}

	while (!prethreadInQueue.empty()) {
		ArchiveOperation op = prethreadInQueue.front();
		if (!threadInQueue.push(op))
			break;
		prethreadInQueue.pop();
	}

	while(!toGenerateQueue.empty()) {
		Chunk *chunk = toGenerateQueue.front();
		if (!asyncWorldGenerator.generateChunk(chunk))
			break;
		toGenerateQueue.pop();
	}

	ArchiveOperation op;
	while (threadOutQueue.pop(op)) {
		switch(op.type) {
		case LOAD:
			if (op.chunk->isInitialized())
				insertLoadedChunk(op.chunk);
			else
				toGenerateQueue.push(op.chunk);
			numSessionChunkLoads++;
			break;
		case STORE:
			recycleChunk(op.chunk);
			break;
		}
	}

	Chunk *chunk;
	while ((chunk = asyncWorldGenerator.getNextChunk()) != nullptr) {
		if (!chunk->isInitialized())
			LOG_WARNING(logger) << "Generator didn't initialize chunk";
		insertReceivedChunk(chunk);
		numSessionChunkGens++;
	}
}

void ServerChunkManager::doWork() {
	ArchiveOperation op;
	if (threadInQueue.pop(op)) {
		switch (op.type) {
		case LOAD:
			archive->loadChunk(op.chunk);
			break;
		case STORE:
			archive->storeChunk(*op.chunk);
			break;
		}
		while (!threadOutQueue.push(op)) {
			sleepFor(millis(50));
		}
	} else {
		sleepFor(millis(100));
	}
}

void ServerChunkManager::onStop() {
	ArchiveOperation op;
	while (threadInQueue.pop(op)) {
		if (op.type == STORE) {
			archive->storeChunk(*op.chunk);
		}
	}
}

void ServerChunkManager::placeBlock(vec3i64 chunkCoords, size_t intraChunkIndex,
		uint blockType, uint32 revision) {
	auto it = chunks.find(chunkCoords);
	if (it != chunks.end()) {
		if (it->second->getRevision() == revision)
			it->second->setBlock(intraChunkIndex, blockType);
		else
			LOG_WARNING(logger) << "couldn't apply chunk patch";
	}
	// TODO operate on cache if chunk is not loaded
}

const Chunk *ServerChunkManager::getChunk(vec3i64 chunkCoords) const {
	auto it = chunks.find(chunkCoords);
	if (it != chunks.end())
		return it->second;
	return nullptr;
}

void ServerChunkManager::requireChunk(vec3i64 chunkCoords) {
	auto it = needCounter.find(chunkCoords);
	if (it == needCounter.end()) {
		requestedQueue.push(chunkCoords);
		needCounter.insert({chunkCoords, 1});
	} else {
		it->second++;
	}
}

void ServerChunkManager::releaseChunk(vec3i64 chunkCoords) {
	auto it1 = needCounter.find(chunkCoords);
	if (it1 != needCounter.end()) {
		it1->second--;
		if (it1->second == 0) {
			needCounter.erase(it1);
			auto it2 = chunks.find(chunkCoords);
			if (it2 != chunks.end()) {
				auto it3 = cacheRevisions.find(chunkCoords);
				if (it3 == cacheRevisions.end() || it2->second->getRevision() != it3->second)
					prethreadInQueue.push(ArchiveOperation{it2->second, STORE});
				else
					recycleChunk(it2->second);
				chunks.erase(it2);
				if (it3 != cacheRevisions.end())
					cacheRevisions.erase(it3);
			}
		}
	}
}

int ServerChunkManager::getNumNeededChunks() const {
	return (int)needCounter.size();
}

int ServerChunkManager::getNumAllocatedChunks() const {
	return (int)(CHUNK_POOL_SIZE - unusedChunks.size());
}

int ServerChunkManager::getNumLoadedChunks() const {
	return (int)chunks.size();
}

int ServerChunkManager::getRequestedQueueSize() const {
	return (int)requestedQueue.size();
}

int ServerChunkManager::getNotInCacheQueueSize() const {
	return (int)toGenerateQueue.size();
}

void ServerChunkManager::insertLoadedChunk(Chunk *chunk) {
	auto it = needCounter.find(chunk->getCC());
	if (it != needCounter.end()) {
		chunks.insert({chunk->getCC(), chunk});
		cacheRevisions.insert({chunk->getCC(), chunk->getRevision()});
	} else {
		recycleChunk(chunk);
	}
}

void ServerChunkManager::insertReceivedChunk(Chunk *chunk) {
	auto it = needCounter.find(chunk->getCC());
	if (it != needCounter.end()) {
		chunks.insert({chunk->getCC(), chunk});
	} else {
		ArchiveOperation op = {chunk, STORE};
		prethreadInQueue.push(op);
	}
}

void ServerChunkManager::recycleChunk(Chunk *chunk) {
	chunk->reset();
	unusedChunks.push(chunk);
}
