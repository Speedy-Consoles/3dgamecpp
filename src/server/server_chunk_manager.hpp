#ifndef SERVER_CHUNK_MANAGER_HPP
#define SERVER_CHUNK_MANAGER_HPP

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
#include "shared/game/world_generator.hpp"
#include "shared/async_world_generator.hpp"
#include "shared/block_utils.hpp"
#include "shared/chunk_archive.hpp"

class ServerChunkManager : public ChunkManager, public Thread {
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
	std::unordered_map<vec3i64, uint32, size_t(*)(vec3i64)> oldRevisions;
	std::unordered_map<vec3i64, int, size_t(*)(vec3i64)> needCounter;

	int numSessionChunkLoads = 0;
	int numSessionChunkGens = 0;

	std::unique_ptr<WorldGenerator> worldGenerator;
	AsyncWorldGenerator asyncWorldGenerator;

	std::unique_ptr<ChunkArchive> archive;

public:
	ServerChunkManager(std::unique_ptr<WorldGenerator> worldGenerator,
			std::unique_ptr<ChunkArchive> archive);
	virtual ~ServerChunkManager();

	void tick();
	virtual void doWork() override;
	virtual void onStop() override;
	void storeChunks();

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
	bool insertLoadedChunk(Chunk *chunk);
	void recycleChunk(Chunk *chunk);
};

#endif /* SERVER_CHUNK_MANAGER_HPP */
