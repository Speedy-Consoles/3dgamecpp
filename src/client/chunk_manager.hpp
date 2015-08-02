#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <unordered_map>
#include <memory>

#include "util.hpp"
#include "engine/vmath.hpp"
#include "engine/queue.hpp"
#include "io/archive.hpp"
#include "game/chunk.hpp"

class ChunkManager {
	static const int MAX_LISTENERS = 2;

	struct Subscription {
		vec3i64 chunkCoords;
		bool subscribe;
		int listenerId;
	};

	std::unordered_map<vec3i64, int, size_t(*)(vec3i64)> chunkListeners;
	ProducerQueue<std::shared_ptr<const Chunk>> *outQueues[MAX_LISTENERS];
	ProducerQueue<Subscription> inQueue;
	ChunkArchive archive;
	std::atomic<bool> shouldHalt;

public:
	ChunkManager();
	~ChunkManager();

	void subscribe(vec3i64 chunkCoords, int listenerId);
	void unsubscribe(vec3i64 chunkCoords, int listenerId);

	std::shared_ptr<const Chunk> getNextChunk(int listenerId);

	void run();
	void requestTermination();
};

#endif /* CHUNK_MANAGER_HPP */
