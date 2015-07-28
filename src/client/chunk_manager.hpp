#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <unordered_map>

#include "util.hpp"
#include "engine/vmath.hpp"
#include "engine/queue.hpp"

class ChunkManager {
	const int MAX_LISTENERS = 2;

	struct Subscription {
		vec3i64 chunkCoords;
		bool unsubscribe;
		int listenerId;
	};

	std::unordered_map<vec3i64, int, vec3i64HashFunc> chunkListeners;
	ProducerQueue<shared_ptr<const Chunk>> *outQueues[MAX_LISTENERS];
	ProducerQueue<Subscription> inQueue;

public:
	ChunkManager();
	~ChunkManager();

	void subscribe(vec3i64 chunkCoords, int listenerId);
	void unsubscribe(vec3i64 chunkCoords, int listenerId);

	shared_ptr<const Chunk> getNextChunk(int listenerId);

	void run();
};

#endif /* CHUNK_MANAGER_HPP */
