#ifndef CLIENT_CHUNK_MANAGER_HPP
#define CLIENT_CHUNK_MANAGER_HPP

#include <memory>
#include <atomic>
#include <future>
#include <queue>
#include <stack>

#include "shared/chunk_manager.hpp"

#include "shared/engine/vmath.hpp"
#include "shared/engine/queue.hpp"
#include "shared/engine/thread.hpp"
#include "shared/game/chunk.hpp"
#include "shared/block_utils.hpp"
#include "shared/chunk_archive.hpp"

class Client;

class ClientChunkManager : public ChunkManager, public Thread {
public:
	static const int CHUNK_POOL_SIZE = 20000;

private:
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
	std::unordered_map<vec3i64, uint32, size_t(*)(vec3i64)> cacheRevisions;
	std::unordered_map<vec3i64, int, size_t(*)(vec3i64)> needCounter;

	int numSessionChunkLoads = 0;
	int numSessionChunkGens = 0;

	Client *client = nullptr;

	std::unique_ptr<ChunkArchive> archive;

public:
	ClientChunkManager(Client *client, std::unique_ptr<ChunkArchive> archive);
	virtual ~ClientChunkManager();

	void tick();
	virtual void doWork() override;
	virtual void onStop() override;

	void placeBlock(vec3i64 chunkCoords, size_t intraChunkIndex,
			uint blockType, uint32 revision);

	virtual const Chunk *getChunk(vec3i64 chunkCoords) const override;
	virtual void requestChunk(vec3i64 chunkCoords) override;
	virtual void releaseChunk(vec3i64 chunkCoords) override;

	int getNumNeededChunks() const;
	int getNumAllocatedChunks() const;
	int getNumLoadedChunks() const;

	int getRequestedQueueSize() const;
	int getNotInCacheQueueSize() const;

	int getNumSessionChunkLoads() const { return numSessionChunkLoads; }
	int getNumSessionChunkGens() const { return numSessionChunkGens; }

private:
	void insertLoadedChunk(Chunk *chunk);
	void recycleChunk(Chunk *chunk);
};

#endif /* CLIENT_CHUNK_MANAGER_HPP */
