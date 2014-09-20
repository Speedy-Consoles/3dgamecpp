#ifndef CHUNK_LOADER_HPP
#define CHUNK_LOADER_HPP

#include "perlin.hpp"
#include "vmath.hpp"
#include "world.hpp"
#include "queue.hpp"
#include "stack.hpp"
#include "archive.hpp"
#include "world_generator.hpp"

#include <atomic>
#include <future>
#include <stack>

class ChunkLoader {
private:
	static const int MAX_LOADS_UNTIL_UNLOAD = 200;

	bool updateFaces;
	uint localPlayer;

	WorldGenerator *gen = nullptr;

	World *world = nullptr;

	std::unordered_set<vec3i64, size_t(*)(vec3i64)> isLoaded;
	std::atomic<bool> shouldHalt;

	ProducerQueue<Chunk *> queue;
	ProducerStack<vec3i64> unloadQueries;
	ProducerStack<Chunk *> deletedChunks;

	std::future<void> fut;

	ChunkArchive chunkArchive;

public:
	ChunkLoader() = delete;
	ChunkLoader(World *world, uint64 seed, uint localPlayer);
	~ChunkLoader();

	// start and stop the asynchronous chunk loader
	void dispatch();
	void requestTermination();
	void wait();

	// these two are used by the client to retrieve data in a threadsafe way
	Chunk *getNextLoadedChunk();
	ProducerStack<vec3i64>::Node *getUnloadQueries();

	// chunks call this themselves, when they are free()-ed.
	void free(Chunk *chunk) { deletedChunks.push(chunk); };

	void setRenderDistance(uint i);
	uint getRenderDistance() { return renderDistance; };


private:
	void run();

	void updateRenderDistance();

	void storeChunksOnDisk();
	void sendOffloadQueries();


	bool loadNextChunk();
	vec3i64 getNextChunkToLoad();
	void tryToLoadChunk(vec3i64);
	bool updatePlayerInfo(bool wait = true);

	std::vector<Chunk *> chunkPool;
	Chunk *allocateChunk(vec3i64);
	void deallocateChunk(Chunk *);
	void clearChunkPool();

	std::atomic<uint> renderDistance;
	std::atomic<uint> newRenderDistance;

	vec3i64 lastPcc;
	bool isPlayerValid;
	uint playerChunkIndex;
	uint playerChunksLoaded;
};

#endif // CHUNK_LOADER_HPP
