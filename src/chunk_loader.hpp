#ifndef CHUNK_LOADER_HPP
#define CHUNK_LOADER_HPP

#include "perlin.hpp"
#include "vmath.hpp"
#include "world.hpp"
#include "queue.hpp"
#include "stack.hpp"
#include "archive.hpp"

#include <atomic>
#include <future>

class ChunkLoader {
private:
	static const int MAX_LOADS_UNTIL_UNLOAD = 200;

	Perlin perlin;

	double overAllScale = 1;

	double perlinAreaXYScale = 6000;
	double perlinAreaMountainThreshold = 0.7;
	double perlinAreaSharpness = 20;

	double perlinMountainXYScale = 500;
	double perlinMountainMaxHeight = 800;
	int perlinMountainOctaves = 8;
	double perlinMountainExp = 0.5;

	double perlinFlatlandXYScale = 800;
	double perlinFlatLandMaxHeight = 40;
	int perlinFlatlandOctaves = 6;
	double perlinFlatlandExp = 0.8;

	double perlinCaveScale = 100;

	bool updateFaces;

	World *world;

	std::unordered_set<vec3i64, size_t(*)(vec3i64)> isLoaded;
	std::atomic<bool> shouldHalt;

	ProducerQueue<Chunk *> queue;
	ProducerStack<vec3i64> unloadQueries;
	ProducerStack<Chunk *> deletedChunks;

	std::future<void> fut;

	ChunkArchive chunkArchive;

public:
	ChunkLoader() = delete;
	ChunkLoader(World *world, uint64 seed, bool updateFaces);
	~ChunkLoader();

	void dispatch();
	void requestTermination();
	void wait();

	Chunk *next();
	void free(Chunk *chunk) { deletedChunks.push(chunk); };

	ProducerStack<vec3i64>::Node *getUnloadQueries();

private:
	void run();
	Chunk *generateChunk(vec3i64 cc);
	void storeChunksOnDisk();
	void sendOffloadQueries();
	bool updatePlayerInfo(uint8, bool wait = true);

	vec3i64 lastPcc[MAX_CLIENTS];
	bool isPlayerValid[MAX_CLIENTS];
	uint playerChunkIndex[MAX_CLIENTS];
	uint playerChunksLoaded[MAX_CLIENTS];
};

#endif // CHUNK_LOADER_HPP
