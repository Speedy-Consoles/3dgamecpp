#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <unordered_map>
#include <memory>
#include <atomic>
#include <future>

#include "util.hpp"
#include "engine/vmath.hpp"
#include "engine/queue.hpp"
#include "io/archive.hpp"
#include "game/chunk.hpp"

class ChunkManager {
	static const int MAX_LISTENERS = 2;

	struct Request {
		vec3i64 chunkCoords;
		int listenerId;
	};

	std::unordered_map<vec3i64, int, size_t(*)(vec3i64)> chunkListeners;
	ProducerQueue<std::shared_ptr<const Chunk>> *outQueues[MAX_LISTENERS];
	ProducerQueue<Request> inQueue;
	ChunkArchive archive;

	std::atomic<bool> shouldHalt;
	std::future<void> fut;

public:
	ChunkManager();
	~ChunkManager();

	void dispatch();

	bool request(vec3i64 chunkCoords, int listenerId);

	std::shared_ptr<const Chunk> getNextChunk(int listenerId);
	void requestTermination();
	void wait();

private:
	void run();
};

#endif /* CHUNK_MANAGER_HPP */
