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

class ChunkManager {
public:
	ChunkManager() = default;
	~ChunkManager() = default;

	virtual const Chunk *getChunk(vec3i64 chunkCoords) const = 0;
	virtual void requestChunk(vec3i64 chunkCoords) = 0;
	virtual void releaseChunk(vec3i64 chunkCoords) = 0;
};

#endif /* CHUNK_MANAGER_HPP */
