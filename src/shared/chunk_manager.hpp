#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <memory>
#include <atomic>
#include <future>

#include "util.hpp"
#include "engine/vmath.hpp"
#include "engine/queue.hpp"
#include "io/archive.hpp"
#include "game/chunk.hpp"

class Client;

class ChunkManager {
	static const int MAX_LOADED_CHUNKS = 10000;

	std::unordered_map<vec3i64, Chunk *, size_t(*)(vec3i64)> chunks;
	std::unordered_map<vec3i64, int, size_t(*)(vec3i64)> needCounter;

	Client *client = nullptr;

	ChunkArchive archive;

public:
	ChunkManager(Client *client);
	~ChunkManager();

	void tick();

	void placeBlock(vec3i64 blockCoords, uint8 blockType);

	const Chunk *getChunk(vec3i64 chunkCoords) const;
	void requestChunk(vec3i64 chunkCoords);
	void releaseChunk(vec3i64 chunkCoords);

	int getNumNeededChunks() const;
	int getNumLoadedChunks() const;
};

#endif /* CHUNK_MANAGER_HPP */
