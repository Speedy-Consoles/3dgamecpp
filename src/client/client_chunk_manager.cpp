#include "client_chunk_manager.hpp"

#include "shared/engine/logging.hpp"
#include "shared/engine/time.hpp"
#include "shared/game/world.hpp"
#include "client/client.hpp"
#include "client/server_interface.hpp"

using namespace std;

static logging::Logger logger("ccm");

ClientChunkManager::ClientChunkManager(Client *client, std::unique_ptr<ChunkArchive> archive) :
	threadOutQueue(1024),
	threadInQueue(1024),
	chunks(0, vec3i64HashFunc),
	cachedRevisions(0, vec3i64HashFunc),
	needCounter(0, vec3i64HashFunc),
	client(client),
	archive(std::move(archive))
{
	for (int i = 0; i < CHUNK_POOL_SIZE; i++) {
		chunkPool[i] = new Chunk(Chunk::ChunkFlags::VISUAL);
		unusedChunks.push(chunkPool[i]);
	}
	dispatch();
}

ClientChunkManager::~ClientChunkManager() {
	LOG_TRACE(logger) << "Destroying ChunkManager";
	requestTermination();
	ArchiveOperation op;
	while(threadOutQueue.pop(op));
	wait();
	while (!preThreadInQueue.empty()) {
		ArchiveOperation op = preThreadInQueue.front();
		preThreadInQueue.pop();
		if (op.type != STORE)
			continue;
		archive->storeChunk(*op.chunk);
	}
	for (auto it1 = chunks.begin(); it1 != chunks.end(); ++it1) {
		Chunk *chunk = it1->second;
		uint32 revision;
		bool cached = archive->hasChunk(chunk->getCC(), &revision);
		if (!cached || revision != chunk->getRevision())
			archive->storeChunk(*chunk);
	}
	for (int i = 0; i < CHUNK_POOL_SIZE; i++) {
		delete chunkPool[i];
	}
}

void ClientChunkManager::tick() {
	while (!requiredQueue.empty() && !unusedChunks.empty()) {
		vec3i64 cc = requiredQueue.front();
		bool cached;
		uint32 revision = 0;
		auto it = cachedRevisions.find(cc);
		if (it == cachedRevisions.end()) {
			cached = archive->hasChunk(cc, &revision);
		} else {
			cached = true;
			revision = it->second;
		}
		Chunk *chunk = unusedChunks.top();
		chunk->initCC(cc);
		client->getServerInterface()->requestChunk(chunk, cached, revision);
		requiredQueue.pop();
		unusedChunks.pop();
	}

	Chunk *chunk;
	while ((chunk = client->getServerInterface()->getNextChunk()) != nullptr) {
		if (chunk->isInitialized()) {
			insertReceivedChunk(chunk);
			numSessionChunkGens++;
		} else {
			ArchiveOperation op = {chunk, LOAD};
			preThreadInQueue.push(op);
		}
	}

	while (!preThreadInQueue.empty()) {
		ArchiveOperation op = preThreadInQueue.front();
		if (!threadInQueue.push(op))
			break;
		if (op.type == STORE)
			cachedRevisions.insert({op.chunk->getCC(), op.chunk->getRevision()});
		preThreadInQueue.pop();
	}

	ArchiveOperation op;
	while (threadOutQueue.pop(op)) {
		switch(op.type) {
		case LOAD:
			if (op.chunk->isInitialized()) {
				insertLoadedChunk(op.chunk);
				numSessionChunkLoads++;
			} else {
				LOG_ERROR(logger) << "Chunk archive did not load chunk";
			}
			break;
		case STORE:
			cachedRevisions.erase(op.chunk->getCC());
			recycleChunk(op.chunk);
			break;
		default:
			break;
		}
	}
}

void ClientChunkManager::doWork() {
	ArchiveOperation op;
	if (threadInQueue.pop(op)) {
		switch (op.type) {
		case LOAD:
			archive->loadChunk(op.chunk);
			break;
		case STORE:
			archive->storeChunk(*op.chunk);
			break;
		case STORE_SILENTLY:
			archive->storeChunk(*op.chunk);
			break;
		}
		while (op.type != STORE_SILENTLY && !threadOutQueue.push(op)) {
			sleepFor(millis(50));
		}
	} else {
		sleepFor(millis(100));
	}
}

void ClientChunkManager::onStop() {
	ArchiveOperation op;
	while (threadInQueue.pop(op)) {
		if (op.type == STORE) {
			archive->storeChunk(*op.chunk);
		}
	}
}

void ClientChunkManager::placeBlock(vec3i64 chunkCoords, size_t intraChunkIndex,
		uint blockType, uint32 revision) {
	auto it = chunks.find(chunkCoords);
	if (it != chunks.end()) {
		if (it->second->getRevision() == revision)
			it->second->setBlock(intraChunkIndex, blockType);
		else
			LOG_WARNING(logger) << "Couldn't apply chunk patch";
	}
	// TODO operate on cache if chunk is not loaded
}

const Chunk *ClientChunkManager::getChunk(vec3i64 chunkCoords) const {
	auto it = chunks.find(chunkCoords);
	if (it != chunks.end())
		return it->second;
	return nullptr;
}

void ClientChunkManager::requireChunk(vec3i64 chunkCoords) {
	auto it = needCounter.find(chunkCoords);
	if (it == needCounter.end()) {
		requiredQueue.push(chunkCoords);
		needCounter.insert({chunkCoords, 1});
	} else {
		it->second++;
	}
}

void ClientChunkManager::releaseChunk(vec3i64 chunkCoords) {
	auto it1 = needCounter.find(chunkCoords);
	if (it1 != needCounter.end()) {
		it1->second--;
		if (it1->second == 0) {
			needCounter.erase(it1);
			auto it2 = chunks.find(chunkCoords);
			if (it2 != chunks.end()) {
				Chunk *chunk = it2->second;
				chunks.erase(it2);
				uint32 revision;
				bool cached = archive->hasChunk(chunk->getCC(), &revision);
				if (!cached || chunk->getRevision() != revision)
					preThreadInQueue.push(ArchiveOperation{chunk, STORE});
				else
					recycleChunk(chunk);
			}
		}
	}
}

int ClientChunkManager::getNumNeededChunks() const {
	return needCounter.size();
}

int ClientChunkManager::getNumAllocatedChunks() const {
	return CHUNK_POOL_SIZE - unusedChunks.size();
}

int ClientChunkManager::getNumLoadedChunks() const {
	return chunks.size();
}

int ClientChunkManager::getRequiredQueueSize() const {
	return requiredQueue.size();
}

int ClientChunkManager::getNotInCacheQueueSize() const {
	return notInCacheQueue.size();
}

void ClientChunkManager::insertLoadedChunk(Chunk *chunk) {
	auto it = needCounter.find(chunk->getCC());
	if (it != needCounter.end()) {
		chunks.insert({chunk->getCC(), chunk});
	} else {
		recycleChunk(chunk);
	}
}

void ClientChunkManager::insertReceivedChunk(Chunk *chunk) {
	auto it = needCounter.find(chunk->getCC());
	if (it != needCounter.end()) {
		chunks.insert({chunk->getCC(), chunk});
		ArchiveOperation op = {chunk, STORE_SILENTLY};
		preThreadInQueue.push(op);
	} else {
		ArchiveOperation op = {chunk, STORE};
		preThreadInQueue.push(op);
	}
}

void ClientChunkManager::recycleChunk(Chunk *chunk) {
	chunk->reset();
	unusedChunks.push(chunk);
}
