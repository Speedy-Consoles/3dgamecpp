#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <memory>
#include <atomic>
#include <future>
#include <queue>
#include <stack>

#include "engine/vmath.hpp"
#include "engine/queue.hpp"
#include "engine/thread.hpp"
#include "game/chunk.hpp"
#include "block_utils.hpp"
#include "chunk_archive.hpp"

class Client;

class ChunkManager : public Thread {
	static const int CHUNK_POOL_SIZE = 20000;

	enum ArchiveOperationType {
		LOAD = 0,
		STORE,
	};

	struct ArchiveOperation {
		Chunk *chunk;
		ArchiveOperationType type;
	};

	Chunk *chunkPool[CHUNK_POOL_SIZE];
	std::stack<Chunk *> unusedChunks;

	std::queue<vec3i64> requestedQueue;
	std::queue<Chunk *> notInCacheQueue;
	std::queue<Chunk *> preToStoreQueue;
	ProducerQueue<ArchiveOperation> loadedStoredQueue;
	ProducerQueue<ArchiveOperation> toLoadStoreQueue;
	std::unordered_map<vec3i64, Chunk *, size_t(*)(vec3i64)> chunks;
	std::unordered_map<vec3i64, uint32, size_t(*)(vec3i64)> oldRevisions;
	std::unordered_map<vec3i64, int, size_t(*)(vec3i64)> needCounter;

	int numSessionChunkLoads = 0;
	int numSessionChunkGens = 0;

	Client *client = nullptr;

	std::unique_ptr<ChunkArchive> archive;

public:
	ChunkManager(Client *client);
	~ChunkManager();

	void tick();
	virtual void doWork() override;
	virtual void onStop() override;
	void storeChunks();

	void placeBlock(vec3i64 chunkCoords, size_t intraChunkIndex,
			uint blockType, uint32 revision);

	const Chunk *getChunk(vec3i64 chunkCoords) const;
	void requestChunk(vec3i64 chunkCoords);
	void releaseChunk(vec3i64 chunkCoords);

	int getNumNeededChunks() const;
	int getNumAllocatedChunks() const;
	int getNumLoadedChunks() const;

	int getRequestedQueueSize() const;
	int getNotInCacheQueueSize() const;

	int getNumSessionChunkLoads() const { return numSessionChunkLoads; }
	int getNumSessionChunkGens() const { return numSessionChunkGens; }

private:
	bool insertLoadedChunk(Chunk *chunk);
	void recycleChunk(Chunk *chunk);
};

#endif /* CHUNK_MANAGER_HPP */
