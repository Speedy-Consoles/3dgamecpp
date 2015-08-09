#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <memory>
#include <atomic>
#include <future>
#include <queue>

#include "util.hpp"
#include "engine/vmath.hpp"
#include "engine/queue.hpp"
#include "engine/thread.hpp"
#include "io/archive.hpp"
#include "game/chunk.hpp"

class Client;

class ChunkManager : public Thread {
	static const int MAX_ALLOCATED_CHUNKS = 10000;

	int numAllocatedChunks = 0;

	enum ArchiveOperationType {
		LOAD = 0,
		STORE,
	};

	struct ArchiveOperation {
		Chunk *chunk;
		ArchiveOperationType type;
	};

	std::queue<vec3i64> requestedQueue;
	std::queue<Chunk *> notInCacheQueue;
	ProducerQueue<Chunk *> loadedQueue;
	ProducerQueue<ArchiveOperation> toLoadStoreQueue;
	std::unordered_map<vec3i64, Chunk *, size_t(*)(vec3i64)> chunks;
	std::unordered_map<vec3i64, int, size_t(*)(vec3i64)> needCounter;

	Client *client = nullptr;

	ChunkArchive archive;

public:
	ChunkManager(Client *client);
	~ChunkManager();

	void tick();
	virtual void doWork() override;

	void placeBlock(vec3i64 blockCoords, uint8 blockType);

	const Chunk *getChunk(vec3i64 chunkCoords) const;
	void requestChunk(vec3i64 chunkCoords);
	void releaseChunk(vec3i64 chunkCoords);

	int getNumNeededChunks() const;
	int getNumAllocatedChunks() const;
	int getNumLoadedChunks() const;

	int getRequestedQueueSize() const;
	int getNotInCacheQueueSize() const;

private:
	void insertLoadedChunk(Chunk *chunk);
};

#endif /* CHUNK_MANAGER_HPP */
