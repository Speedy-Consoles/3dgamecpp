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

	double areaXYScale = 6000;
	double areaMountainThreshold = 0.7;
	double areaSharpness = 20;

	double mountainXYScale = 500;
	double mountainMaxHeight = 800;
	int mountainOctaves = 8;
	double mountainExp = 0.5;

	double flatlandXYScale = 800;
	double flatLandMaxHeight = 40;
	int flatlandOctaves = 6;
	double flatlandExp = 0.8;

	double surfaceScale = 70;
	double surfaceRelDepth = 0.3;
	double surfaceExp = 0.4;
	double surfaceThresholdXScale = 1;
	double surfaceThresholdYScale;

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
